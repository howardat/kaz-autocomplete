"""Build training/validation corpora from the HF Kazakh dataset.

Each split is saved as an .npz with:
  ids    uint8  russianized char ids of all documents, concatenated
  tags   uint8  target tag per char
  ends   int64  index of the last char of every word
  starts int64  start index of the document each word belongs to

Documents never cross splits.

    uv run python -m ml.data --train-chars 150_000_000
"""

import argparse
import re
import unicodedata
from pathlib import Path

import numpy as np
from datasets import load_dataset

from ml.rules import encode

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
MAX_DOC_CHARS = 4000
WORD_END = re.compile(r"[^\W\d_](?![^\W\d_])")


def clean(text: str) -> str:
    text = unicodedata.normalize("NFC", text).lower()
    return " ".join(text.split())[:MAX_DOC_CHARS]


def build(train_chars: int, val_chars: int, seed: int) -> None:
    ds = load_dataset("kz-transformers/multidomain-kazakh-dataset", split="train").select_columns(
        ["text", "predicted_language", "contains_kaz_symbols"]
    )
    order = np.random.default_rng(seed).permutation(len(ds))
    targets = {"val": val_chars, "train": train_chars}
    parts = {k: {"ids": [], "tags": [], "ends": [], "starts": []} for k in targets}
    sizes = dict.fromkeys(targets, 0)

    batch = 20_000
    for b in range(0, len(order), batch):
        rows = ds[np.sort(order[b : b + batch]).tolist()]
        for text, lang, kaz in zip(
            rows["text"], rows["predicted_language"], rows["contains_kaz_symbols"]
        ):
            if lang != "kaz" or not kaz or not text:
                continue
            split = "val" if sizes["val"] < targets["val"] else "train"
            doc = clean(text)
            ids, tags = encode(doc)
            offset = sizes[split]
            p = parts[split]
            p["ids"].append(ids)
            p["tags"].append(tags)
            ends = np.fromiter((m.start() for m in WORD_END.finditer(doc)), np.int64)
            p["ends"].append(ends + offset)
            p["starts"].append(np.full(len(ends), offset, np.int64))
            sizes[split] += len(ids)
        print(f"rows {b + batch:>10,}  " + "  ".join(f"{k}={v:,}" for k, v in sizes.items()))
        if sizes["train"] >= targets["train"]:
            break

    DATA_DIR.mkdir(exist_ok=True)
    for split, p in parts.items():
        np.savez(DATA_DIR / f"{split}.npz", **{k: np.concatenate(v) for k, v in p.items()})
        print(f"saved {split}: {sizes[split]:,} chars, {sum(map(len, p['ends'])):,} words")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--train-chars", type=int, default=150_000_000)
    ap.add_argument("--val-chars", type=int, default=3_000_000)
    ap.add_argument("--seed", type=int, default=0)
    a = ap.parse_args()
    build(a.train_chars, a.val_chars, a.seed)
