"""Q12 metrics: word accuracy on held-out words with ambiguous letters, plus latency.

    uv run python -m ml.eval            # PyTorch checkpoint
    uv run python -m ml.eval --coreml   # also time the exported Core ML model
"""

import argparse
import time

import numpy as np
import torch

from ml.rules import AMBIGUOUS, MAXLEN, TAG_COUNT
from ml.windows import Corpus


def top_k(ids: np.ndarray, probs: np.ndarray, k: int = 3) -> list[tuple[list[int], float]]:
    """Beam search over one word's per-char tag probs -> k best (tags, log prob)."""
    beams: list[tuple[list[int], float]] = [([], 0.0)]
    for c, p in zip(ids, probs):
        n = TAG_COUNT[c]
        if n == 1:
            beams = [(t + [0], s) for t, s in beams]
            continue
        lp = np.log(np.maximum(p[:n], 1e-9))
        beams = [(t + [j], s + lp[j]) for t, s in beams for j in range(n)]
        beams = sorted(beams, key=lambda b: -b[1])[:k]
    return beams


def eval_words(corpus: Corpus, n: int, seed: int = 1) -> np.ndarray:
    """Fixed sample of word indices that contain at least one ambiguous letter."""
    rng = np.random.default_rng(seed)
    cand = rng.permutation(len(corpus.ends))[: n * 3]
    keep = [
        i for i in cand
        if AMBIGUOUS[corpus.ids[corpus.word_starts[i]:corpus.ends[i] + 1]].any()
    ]
    return np.array(keep[:n])


@torch.no_grad()
def evaluate(model, corpus: Corpus, device: str, n: int = 5000) -> dict[str, float]:
    idx = eval_words(corpus, n)
    out = {}
    for name, ctx in (("", MAXLEN), ("_noctx", 0)):
        ids, tags, wl = corpus.windows(idx, context=ctx)
        # Chunked: large LSTM batches on MPS run out of memory or return garbage.
        probs = np.concatenate([
            torch.softmax(model(torch.from_numpy(c).to(device)), -1).cpu().numpy()
            for c in np.array_split(ids, max(1, len(ids) // 500))
        ])
        top1 = top3 = 0
        for i in range(len(idx)):
            s = MAXLEN - wl[i]
            gold = tags[i, s:].tolist()
            best = [t for t, _ in top_k(ids[i, s:], probs[i, s:], 3)]
            top1 += best[0] == gold
            top3 += gold in best
        out["top1" + name] = top1 / len(idx)
        out["top3" + name] = top3 / len(idx)
    return out


def main() -> None:
    from ml.data import DATA_DIR
    from ml.model import load

    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, default=50_000)
    ap.add_argument("--coreml", action="store_true")
    a = ap.parse_args()

    model = load(DATA_DIR / "model.pt")
    val = Corpus(DATA_DIR / "val.npz")
    m = evaluate(model, val, "cpu", a.n)
    for k, v in m.items():
        print(f"{k:12} {v:.4f}")
    print("target       top1 >= 0.97, top3 >= 0.995:",
          "PASS" if m["top1"] >= 0.97 and m["top3"] >= 0.995 else "FAIL")

    if a.coreml:
        import coremltools as ct

        path = DATA_DIR / "KazTagger.mlpackage"
        ml = ct.models.MLModel(str(path), compute_units=ct.ComputeUnit.CPU_ONLY)
        ids, _, _ = val.windows(eval_words(val, 200), context=MAXLEN)
        ml.predict({"chars": ids[:1].astype(np.int32)})  # warm up
        t = time.perf_counter()
        for row in ids:
            ml.predict({"chars": row[None].astype(np.int32)})
        ms = (time.perf_counter() - t) / len(ids) * 1000
        size = sum(f.stat().st_size for f in path.rglob("*") if f.is_file()) / 1e6
        print(f"coreml latency {ms:.2f} ms/word (target < 10), size {size:.2f} MB (target < 5)")


if __name__ == "__main__":
    main()
