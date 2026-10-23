"""Turn a corpus split into fixed-size, right-aligned model windows.

A window mirrors what the IME sees: some already-typed text, then the word
being typed, which always ends at the last position.
"""

from pathlib import Path

import numpy as np

from ml.rules import CHAR_ID, LATIN, MAXLEN, RUSSIAN

LETTER_IDS = np.array([CHAR_ID[c] for c in RUSSIAN] + [CHAR_ID[LATIN]])


class Corpus:
    def __init__(self, path: Path):
        d = np.load(path)
        self.ids, self.tags, self.ends, self.starts = d["ids"], d["tags"], d["ends"], d["starts"]
        non_letter = np.flatnonzero(~np.isin(self.ids, LETTER_IDS))
        # First char of each word: one past the last non-letter before its end.
        prev = np.searchsorted(non_letter, self.ends) - 1
        self.word_starts = np.where(prev >= 0, non_letter[np.maximum(prev, 0)] + 1, 0)
        self.word_starts = np.maximum(self.word_starts, self.starts)

    def windows(self, idx: np.ndarray, rng: np.random.Generator | None = None,
                context: int | None = None, prefix_p: float = 0.0):
        """Build windows for the words at idx.

        context: chars of preceding text to keep; None = random (training).
        prefix_p: chance to cut the word to a random prefix, as while typing.
        Returns ids, tags (B, MAXLEN) and word_len (B,).
        """
        B = len(idx)
        ws, we, ds = self.word_starts[idx], self.ends[idx] + 1, self.starts[idx]
        if prefix_p and rng is not None:
            cut = rng.random(B) < prefix_p
            lens = we - ws
            new_len = (rng.random(B) * lens).astype(np.int64) + 1
            we = np.where(cut, ws + new_len, we)
        if context is None:
            ctx = rng.integers(0, MAXLEN, B)
            ctx = np.where(rng.random(B) < 0.25, 0, ctx)
        else:
            ctx = np.full(B, context)
        begin = np.maximum(np.maximum(ws - ctx, ds), we - MAXLEN)
        ids = np.zeros((B, MAXLEN), np.int64)
        tags = np.zeros((B, MAXLEN), np.int64)
        for i in range(B):
            n = we[i] - begin[i]
            ids[i, MAXLEN - n:] = self.ids[begin[i]:we[i]]
            tags[i, MAXLEN - n:] = self.tags[begin[i]:we[i]]
        return ids, tags, np.minimum(we - ws, MAXLEN)
