"""Export the trained tagger to Core ML plus the vocab the app needs.

    uv run python -m ml.export

Writes data/KazTagger.mlpackage and copies it with vocab.json into
macos/Resources/.
"""

import json
import shutil
import warnings

import coremltools as ct
import coremltools.optimize.coreml as cto
import numpy as np
import torch

from ml.data import DATA_DIR
from ml.model import ExportWrapper, load
from ml.rules import CHAR_ID, DIGIT, LATIN, MAXLEN, NUM_TAGS, PAD, RULES, TO_RUSSIAN, UNK, VOCAB

RESOURCES = DATA_DIR.parent / "macos" / "Resources"


def main() -> None:
    warnings.filterwarnings("ignore")
    model = ExportWrapper(load(DATA_DIR / "model.pt")).eval()
    example = torch.zeros(1, MAXLEN, dtype=torch.int32)
    traced = torch.jit.trace(model, example)
    ml = ct.convert(
        traced,
        inputs=[ct.TensorType(name="chars", shape=(1, MAXLEN), dtype=np.int32)],
        outputs=[ct.TensorType(name="probs")],
        minimum_deployment_target=ct.target.macOS13,
        compute_precision=ct.precision.FLOAT16,
    )
    # 8-bit weights: 9 MB -> 2.3 MB, same word accuracy on held-out words.
    ml = cto.linear_quantize_weights(
        ml, cto.OptimizationConfig(global_config=cto.OpLinearQuantizerConfig(mode="linear_symmetric"))
    )
    ml.short_description = "Kazakh letter restoration from Russian-keyboard input"
    out = DATA_DIR / "KazTagger.mlpackage"
    shutil.rmtree(out, ignore_errors=True)
    ml.save(str(out))

    # Sanity: Core ML agrees with PyTorch on a real window.
    ids = np.zeros((1, MAXLEN), np.int32)
    word = [CHAR_ID[c] for c in "казакстан"]
    ids[0, -len(word):] = word
    want = model(torch.from_numpy(ids)).detach().numpy()
    got = ml.predict({"chars": ids})["probs"]
    print("max |coreml - torch| =", float(np.abs(want - got).max()))

    vocab = {
        "maxlen": MAXLEN,
        "num_tags": NUM_TAGS,
        "vocab": VOCAB,
        "special": {"pad": CHAR_ID[PAD], "unk": CHAR_ID[UNK], "digit": CHAR_ID[DIGIT], "latin": CHAR_ID[LATIN]},
        "rules": RULES,
        "to_russian": {k: v for k, (v, _) in TO_RUSSIAN.items() if k != v},
    }
    RESOURCES.mkdir(parents=True, exist_ok=True)
    (RESOURCES / "vocab.json").write_text(json.dumps(vocab, ensure_ascii=False, indent=1), encoding="utf-8")
    shutil.rmtree(RESOURCES / out.name, ignore_errors=True)
    shutil.copytree(out, RESOURCES / out.name)
    print(f"wrote {out} and {RESOURCES}")


if __name__ == "__main__":
    main()
