"""Character tagger: for every input char, a probability per variant tag."""

import numpy as np
import torch
import torch.nn as nn

from ml.rules import NUM_TAGS, TAG_COUNT, VOCAB


class Tagger(nn.Module):
    def __init__(self, emb: int = 64, hidden: int = 256, layers: int = 2, dropout: float = 0.1):
        super().__init__()
        self.config = dict(emb=emb, hidden=hidden, layers=layers, dropout=dropout)
        self.embed = nn.Embedding(len(VOCAB), emb, padding_idx=0)
        self.lstm = nn.LSTM(
            emb, hidden, layers, batch_first=True, bidirectional=True, dropout=dropout
        )
        self.out = nn.Linear(2 * hidden, NUM_TAGS)
        # Tags past a letter's variant count are impossible; masked to -inf.
        valid = torch.arange(NUM_TAGS)[None, :] < torch.from_numpy(TAG_COUNT)[:, None]
        self.register_buffer("valid", valid, persistent=False)

    def forward(self, ids: torch.Tensor) -> torch.Tensor:
        """ids (B, T) -> masked logits (B, T, NUM_TAGS)."""
        h, _ = self.lstm(self.embed(ids))
        logits = self.out(h)
        return logits.masked_fill(~self.valid[ids], float("-inf"))


class ExportWrapper(nn.Module):
    """What ships in the app: int ids in, probabilities out."""

    def __init__(self, tagger: Tagger):
        super().__init__()
        self.tagger = tagger

    def forward(self, ids: torch.Tensor) -> torch.Tensor:
        return torch.softmax(self.tagger(ids.long()), dim=-1)


def load(path) -> Tagger:
    ckpt = torch.load(path, map_location="cpu")
    model = Tagger(**ckpt["config"])
    model.load_state_dict(ckpt["state"])
    return model.eval()


def param_count(model: nn.Module) -> int:
    return int(sum(np.prod(p.shape) for p in model.parameters()))
