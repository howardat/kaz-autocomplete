"""Export the trained tagger for the Windows app as one binary file.

    uv run python -m ml.export_win

Writes windows/Resources/kaztype.bin. The Windows app runs the BiLSTM itself
(windows/core/Tagger.cpp), so this file holds the vocab, the letter rules and
the weights in the layout that code expects:

  "KZT1"  u32 maxlen num_tags hidden layers
  u32 n, then n strings              vocab
  u32 pad unk digit latin            special ids
  u32 n, then n x (string src, u32 k, k strings)   rules
  layer 0, per direction (fwd, bwd):
    table  f32 [vocab, 4H]           embed @ W_ih.T + b_ih + b_hh
    W_hh   q8  [4H, H]
  layer l > 0, per direction:
    W_ih   q8  [4H, 2H]
    bias   f32 [4H]                  b_ih + b_hh
    W_hh   q8  [4H, H]
  out W    f32 [num_tags, 2H]
  out b    f32 [num_tags]

string = u32 byte length + UTF-8. f32 = u32 rows, u32 cols, data.
q8 = u32 rows, u32 cols, f32 scale per row, int8 data (symmetric per row).
Gate order is PyTorch's: i, f, g, o.
"""

import struct

import numpy as np
import torch
import torch.nn as nn

from ml.data import DATA_DIR
from ml.eval import evaluate
from ml.model import Tagger, load
from ml.rules import CHAR_ID, DIGIT, LATIN, MAXLEN, NUM_TAGS, PAD, RULES, UNK, VOCAB
from ml.windows import Corpus

OUT = DATA_DIR.parent / "windows" / "Resources" / "kaztype.bin"


def quantize(w: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    scale = np.abs(w).max(axis=1) / 127
    scale[scale == 0] = 1
    return scale.astype(np.float32), np.round(w / scale[:, None]).astype(np.int8)


class Writer:
    def __init__(self):
        self.parts: list[bytes] = []

    def u32(self, *vs: int) -> None:
        self.parts.append(struct.pack(f"<{len(vs)}I", *vs))

    def string(self, s: str) -> None:
        b = s.encode("utf-8")
        self.u32(len(b))
        self.parts.append(b)

    def f32(self, a: np.ndarray) -> None:
        a = np.atleast_2d(a).astype("<f4")
        self.u32(*a.shape)
        self.parts.append(a.tobytes())

    def q8(self, w: np.ndarray) -> np.ndarray:
        """Writes w quantized; returns the dequantized weights for checking."""
        scale, q = quantize(w)
        self.u32(*w.shape)
        self.parts.append(scale.astype("<f4").tobytes())
        self.parts.append(q.tobytes())
        return q.astype(np.float32) * scale[:, None]


class AppTagger(nn.Module):
    """The arithmetic of windows/core/Tagger.cpp, for measuring its accuracy:
    int8 weights, and LSTM outputs rounded to steps of 1/127 before they feed
    the next matmul."""

    def __init__(self, model: Tagger, deq: dict[str, np.ndarray]):
        super().__init__()
        self.model, self.deq = model, {k: torch.from_numpy(v) for k, v in deq.items()}

    @staticmethod
    def q(h: torch.Tensor) -> torch.Tensor:
        return torch.round(h.clamp(-1, 1) * 127) / 127

    def forward(self, ids: torch.Tensor) -> torch.Tensor:
        d, layers = self.deq, self.model.config["layers"]
        x = self.model.embed(ids)
        B, T, _ = x.shape
        for layer in range(layers):
            outs = []
            for sfx in ("", "_reverse"):
                p = f"lstm.{{}}_l{layer}{sfx}"
                pre = x @ d[p.format("weight_ih")].T + d[p.format("bias_ih")] + d[p.format("bias_hh")]
                H = d[p.format("weight_hh")].shape[1]
                h, c = torch.zeros(B, H), torch.zeros(B, H)
                out = torch.zeros(B, T, H)
                for t in (range(T - 1, -1, -1) if sfx else range(T)):
                    i, f, g, o = (pre[:, t] + self.q(h) @ d[p.format("weight_hh")].T).chunk(4, -1)
                    c = torch.sigmoid(f) * c + torch.sigmoid(i) * torch.tanh(g)
                    h = out[:, t] = torch.sigmoid(o) * torch.tanh(c)
                outs.append(out)
            x = torch.cat(outs, -1)
            if layer < layers - 1:
                x = self.q(x)
        logits = self.model.out(x)
        return logits.masked_fill(~self.model.valid[ids], float("-inf"))


def main() -> None:
    model = load(DATA_DIR / "model.pt")
    cfg = model.config
    sd = {k: v.numpy() for k, v in model.state_dict().items()}
    deq = {k: v.copy() for k, v in sd.items()}  # what the app will compute with

    w = Writer()
    w.parts.append(b"KZT1")
    w.u32(MAXLEN, NUM_TAGS, cfg["hidden"], cfg["layers"])
    w.u32(len(VOCAB))
    for s in VOCAB:
        w.string(s)
    w.u32(CHAR_ID[PAD], CHAR_ID[UNK], CHAR_ID[DIGIT], CHAR_ID[LATIN])
    w.u32(len(RULES))
    for src, vs in RULES.items():
        w.string(src)
        w.u32(len(vs))
        for v in vs:
            w.string(v)

    emb = sd["embed.weight"]
    for layer in range(cfg["layers"]):
        for sfx in ("", "_reverse"):
            p = f"lstm.{{}}_l{layer}{sfx}"
            bias = sd[p.format("bias_ih")] + sd[p.format("bias_hh")]
            if layer == 0:
                w.f32(emb @ sd[p.format("weight_ih")].T + bias)
            else:
                deq[p.format("weight_ih")] = w.q8(sd[p.format("weight_ih")])
                w.f32(bias)
            deq[p.format("weight_hh")] = w.q8(sd[p.format("weight_hh")])
    w.f32(sd["out.weight"])
    w.f32(sd["out.bias"])

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(b"".join(w.parts))
    print(f"wrote {OUT} ({OUT.stat().st_size / 1e6:.2f} MB)")

    # Accuracy of what the app computes, against the float model.
    app = AppTagger(model, deq).eval()
    val = Corpus(DATA_DIR / "val.npz")
    for name, m in (("float", model), ("app", app)):
        r = evaluate(m, val, "cpu", n=5000)
        print(f"{name:6} " + "  ".join(f"{k} {v:.4f}" for k, v in r.items()))


if __name__ == "__main__":
    main()
