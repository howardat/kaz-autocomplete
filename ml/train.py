"""Train the char tagger.

    uv run python -m ml.train --steps 60000
"""

import argparse
import math
import time

import numpy as np
import torch
import torch.nn.functional as F

from ml.data import DATA_DIR
from ml.eval import evaluate
from ml.model import Tagger, param_count
from ml.rules import AMBIGUOUS
from ml.windows import Corpus

CKPT = DATA_DIR / "model.pt"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--steps", type=int, default=60_000)
    ap.add_argument("--batch", type=int, default=512)
    ap.add_argument("--lr", type=float, default=2e-3)
    ap.add_argument("--hidden", type=int, default=256)
    ap.add_argument("--layers", type=int, default=2)
    ap.add_argument("--eval-every", type=int, default=2000)
    a = ap.parse_args()

    device = "mps" if torch.backends.mps.is_available() else "cpu"
    rng = np.random.default_rng(0)
    train, val = Corpus(DATA_DIR / "train.npz"), Corpus(DATA_DIR / "val.npz")
    ambiguous = torch.from_numpy(AMBIGUOUS).to(device)

    model = Tagger(hidden=a.hidden, layers=a.layers).to(device)
    print(f"{param_count(model):,} params, {len(train.ends):,} train words, device={device}")
    opt = torch.optim.AdamW(model.parameters(), lr=a.lr, weight_decay=0.01)
    warmup = 1000
    sched = torch.optim.lr_scheduler.LambdaLR(
        opt, lambda s: min(1, (s + 1) / warmup) * 0.5 * (1 + math.cos(math.pi * min(1, s / a.steps)))
    )

    best, t0 = 0.0, time.time()
    for step in range(1, a.steps + 1):
        model.train()
        idx = rng.integers(0, len(train.ends), a.batch)
        ids, tags, _ = train.windows(idx, rng, prefix_p=0.2)
        ids, tags = torch.from_numpy(ids).to(device), torch.from_numpy(tags).to(device)
        logits = model(ids)
        mask = ambiguous[ids]
        loss = F.cross_entropy(logits[mask], tags[mask])
        opt.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        opt.step()
        sched.step()

        if step % 200 == 0:
            rate = step * a.batch / (time.time() - t0)
            print(f"step {step:>6}  loss {loss.item():.4f}  lr {sched.get_last_lr()[0]:.2e}  {rate:,.0f} win/s", flush=True)
        if step % a.eval_every == 0 or step == a.steps:
            m = evaluate(model.eval(), val, device, n=5000)
            print("  eval " + "  ".join(f"{k} {v:.4f}" for k, v in m.items()), flush=True)
            if m["top1"] > best:
                best = m["top1"]
                torch.save({"config": model.config, "state": model.state_dict(), "metrics": m}, CKPT)
                print(f"  saved {CKPT} (top1 {best:.4f})", flush=True)


if __name__ == "__main__":
    main()
