"""Letter rules and the character vocabulary shared by training and the macOS app.

Everything the app needs to reproduce model input is exported to vocab.json by
ml/export.py, so keep this the single source of truth.
"""

from pathlib import Path

import numpy as np
import yaml

ROOT = Path(__file__).resolve().parent.parent
RULES_PATH = ROOT / "transliteration_rules.yaml"

# Fixed model window. Input is right-aligned: the current word ends at the last
# position, left padding fills the rest.
MAXLEN = 64
NUM_TAGS = 3  # max variants per letter (у -> у/ұ/ү)


def load_rules() -> dict[str, list[str]]:
    rules = yaml.safe_load(RULES_PATH.read_text(encoding="utf-8"))
    for src, variants in rules.items():
        assert variants[0] == src, f"first variant of {src} must be itself"
        assert len(variants) <= NUM_TAGS
    return rules


RULES = load_rules()

# Kazakh letter -> (Russian-keyboard letter, tag)
TO_RUSSIAN: dict[str, tuple[str, int]] = {
    v: (src, tag) for src, vs in RULES.items() for tag, v in enumerate(vs)
}

PAD, UNK, DIGIT, LATIN = "<pad>", "<unk>", "<digit>", "<latin>"
RUSSIAN = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
PUNCT = " .,!?;:-–—\"'«»()/%№"
VOCAB: list[str] = [PAD, UNK, DIGIT, LATIN, *RUSSIAN, *PUNCT]
CHAR_ID = {c: i for i, c in enumerate(VOCAB)}

# Number of valid tags per input id; 1 for letters that never change.
TAG_COUNT = np.ones(len(VOCAB), dtype=np.int64)
for src, vs in RULES.items():
    TAG_COUNT[CHAR_ID[src]] = len(vs)
AMBIGUOUS = TAG_COUNT > 1


def char_id(c: str) -> int:
    """Map one already-russianized, lowercased char to a vocab id."""
    if c in CHAR_ID:
        return CHAR_ID[c]
    if c.isdigit():
        return CHAR_ID[DIGIT]
    if "a" <= c <= "z":
        return CHAR_ID[LATIN]
    if c.isspace():
        return CHAR_ID[" "]
    return CHAR_ID[UNK]


def russianize(text: str) -> tuple[str, list[int]]:
    """Lowercased Kazakh text -> (what a Russian keyboard would type, per-char tag)."""
    out, tags = [], []
    for c in text:
        src, tag = TO_RUSSIAN.get(c, (c, 0))
        out.append(src)
        tags.append(tag)
    return "".join(out), tags


def encode(text: str) -> tuple[np.ndarray, np.ndarray]:
    """Lowercased Kazakh text -> (input ids, target tags) as uint8 arrays."""
    rus, tags = russianize(text)
    ids = np.fromiter((char_id(c) for c in rus), dtype=np.uint8, count=len(rus))
    return ids, np.asarray(tags, dtype=np.uint8)


def apply_tags(russian_word: str, tags: list[int]) -> str:
    return "".join(
        RULES[c][t] if c in RULES else c for c, t in zip(russian_word, tags)
    )
