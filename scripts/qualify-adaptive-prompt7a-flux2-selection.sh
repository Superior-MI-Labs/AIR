#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODEL_ROOT="${1:-$HOME/Models/Media/Image/FLUX.2-Klein-4B}"
COMFY_ROOT="${2:-$HOME/Projects/AI-Runtimes/ComfyUI}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P7A_FLUX_OUT:-$HOME/Downloads/AIR-0.11-Prompt7A-FLUX2-Selection-$STAMP}"

DIFFUSION="$MODEL_ROOT/flux-2-klein-4b-fp8.safetensors"
ENCODER="$MODEL_ROOT/split_files/text_encoders/qwen_3_4b_fp4_flux2.safetensors"
VAE="$MODEL_ROOT/split_files/vae/flux2-vae.safetensors"

EXPECTED_DIFFUSION_SHA256="97ed34fe0567e436200f2faee3939b88f2b5d99f8af2a4dc16532c4245c0ccb6"
EXPECTED_ENCODER_SHA256="3eab03a77adb0ee5304a4e677d5c10ac22f9049c1d7c894adca4f8bb39206ca8"
EXPECTED_VAE_SHA256="d64f3a68e1cc4f9f4e29b6e0da38a0204fe9a49f2d4053f0ec1fa1ca02f9c4b5"

mkdir -p "$OUT"
cd "$ROOT"

echo "=== PROMPT 7A FLUX.2 KLEIN 4B SELECTION PREFLIGHT ==="
echo "output=$OUT"
echo

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 7A selection requires a clean AIR worktree." >&2
    git status --short >&2
    exit 2
fi

for path in "$DIFFUSION" "$ENCODER" "$VAE"; do
    if [[ ! -f "$path" ]]; then
        echo "ERROR: required FLUX.2 artifact missing: $path" >&2
        exit 3
    fi
done

{
    echo "date=$(date -Is)"
    echo "air_branch=$(git branch --show-current)"
    echo "air_head=$(git rev-parse HEAD)"
    echo "host=$(hostname)"
    echo "model_root=$MODEL_ROOT"
    echo "comfy_root=$COMFY_ROOT"
    echo "diffusion_source=https://huggingface.co/black-forest-labs/FLUX.2-klein-4b-fp8"
    echo "encoder_source=https://huggingface.co/Comfy-Org/vae-text-encorder-for-flux-klein-4b"
    echo "vae_source=https://huggingface.co/Comfy-Org/flux2-dev"
    echo "comfy_workflow_source=https://github.com/Comfy-Org/workflow_templates"
    echo "bfl_reference_source=https://github.com/black-forest-labs/flux2"
} > "$OUT/identity.txt"

echo "=== LOCAL ARTIFACT HASHES ==="
sha256sum "$DIFFUSION" "$ENCODER" "$VAE" | tee "$OUT/artifact-sha256.txt"

ACTUAL_DIFFUSION_SHA256="$(awk -v p="$DIFFUSION" '$2 == p {print $1}' "$OUT/artifact-sha256.txt")"
ACTUAL_ENCODER_SHA256="$(awk -v p="$ENCODER" '$2 == p {print $1}' "$OUT/artifact-sha256.txt")"
ACTUAL_VAE_SHA256="$(awk -v p="$VAE" '$2 == p {print $1}' "$OUT/artifact-sha256.txt")"

[[ -n "$ACTUAL_DIFFUSION_SHA256" && -n "$ACTUAL_ENCODER_SHA256" && -n "$ACTUAL_VAE_SHA256" ]] || {
    echo "ERROR: failed to recover one or more retained artifact hashes." >&2
    exit 4
}

[[ "$ACTUAL_DIFFUSION_SHA256" == "$EXPECTED_DIFFUSION_SHA256" ]] || {
    echo "ERROR: FLUX.2 diffusion SHA256 mismatch." >&2
    exit 4
}
[[ "$ACTUAL_ENCODER_SHA256" == "$EXPECTED_ENCODER_SHA256" ]] || {
    echo "ERROR: Qwen3-4B FP4 text encoder SHA256 mismatch." >&2
    exit 5
}
[[ "$ACTUAL_VAE_SHA256" == "$EXPECTED_VAE_SHA256" ]] || {
    echo "ERROR: FLUX.2 VAE SHA256 mismatch." >&2
    exit 6
}

echo "artifact_identity=PASS"

echo
echo "=== LOCAL ARTIFACT SIZES ==="
stat -c '%n %s bytes' "$DIFFUSION" "$ENCODER" "$VAE" \
    | tee "$OUT/artifact-sizes.txt"

echo
echo "=== SAFETENSORS HEADER CENSUS ==="
python3 - "$DIFFUSION" "$ENCODER" "$VAE" "$OUT/safetensors-census.json" <<'PY'
import collections
import json
import pathlib
import struct
import sys

paths = [pathlib.Path(x) for x in sys.argv[1:4]]
out = pathlib.Path(sys.argv[4])

rows = []
for path in paths:
    with path.open("rb") as fh:
        raw_len = fh.read(8)
        if len(raw_len) != 8:
            raise SystemExit(f"invalid safetensors header length: {path}")
        header_len = struct.unpack("<Q", raw_len)[0]
        if header_len <= 0 or header_len > 256 * 1024 * 1024:
            raise SystemExit(f"implausible safetensors header size {header_len}: {path}")
        header = json.loads(fh.read(header_len))

    metadata = header.pop("__metadata__", {})
    dtype_counts = collections.Counter()
    prefix_counts = collections.Counter()
    tensor_bytes = 0
    example_shapes = []

    for name, info in header.items():
        dtype_counts[str(info.get("dtype"))] += 1
        prefix = name.split(".", 1)[0]
        prefix_counts[prefix] += 1
        offsets = info.get("data_offsets", [0, 0])
        if len(offsets) == 2:
            tensor_bytes += int(offsets[1]) - int(offsets[0])
        if len(example_shapes) < 30:
            example_shapes.append(
                {
                    "name": name,
                    "dtype": info.get("dtype"),
                    "shape": info.get("shape"),
                }
            )

    rows.append(
        {
            "path": str(path),
            "file_bytes": path.stat().st_size,
            "header_bytes": header_len,
            "tensor_count": len(header),
            "tensor_data_bytes": tensor_bytes,
            "dtype_counts": dict(sorted(dtype_counts.items())),
            "top_prefixes": prefix_counts.most_common(40),
            "metadata": metadata,
            "examples": example_shapes,
        }
    )

out.write_text(json.dumps(rows, indent=2, sort_keys=True) + "\n")
for row in rows:
    print(f"--- {pathlib.Path(row['path']).name} ---")
    print(f"tensors={row['tensor_count']}")
    print(f"dtypes={row['dtype_counts']}")
    print(f"header_bytes={row['header_bytes']}")
    print(f"tensor_data_gib={row['tensor_data_bytes'] / 1024**3:.3f}")
    print(f"top_prefixes={row['top_prefixes'][:12]}")
PY

echo
echo "=== COMFYUI PINNED RUNTIME ==="
if [[ ! -d "$COMFY_ROOT/.git" ]]; then
    echo "ERROR: ComfyUI git checkout not found: $COMFY_ROOT" >&2
    exit 7
fi

COMFY_HEAD="$(git -C "$COMFY_ROOT" rev-parse HEAD)"
{
    echo "git_head=$COMFY_HEAD"
    echo "git_branch=$(git -C "$COMFY_ROOT" branch --show-current)"
    echo "git_status_begin"
    git -C "$COMFY_ROOT" status --short
    echo "git_status_end"
} | tee "$OUT/comfy-git.txt"

echo
echo "=== COMFYUI NATIVE FLUX2 SUPPORT ==="
NODES_FLUX="$COMFY_ROOT/comfy_extras/nodes_flux.py"
SUPPORTED_MODELS="$COMFY_ROOT/comfy/supported_models.py"

[[ -f "$NODES_FLUX" ]] || {
    echo "ERROR: missing native ComfyUI Flux nodes source." >&2
    exit 8
}

grep -n 'class EmptyFlux2LatentImage' "$NODES_FLUX" \
    | tee "$OUT/comfy-flux2-latent-node.txt"
grep -n 'class Flux2Scheduler' "$NODES_FLUX" \
    | tee "$OUT/comfy-flux2-scheduler-node.txt"

if [[ -f "$SUPPORTED_MODELS" ]]; then
    grep -ni 'flux2\|flux.2' "$SUPPORTED_MODELS" \
        > "$OUT/comfy-flux2-supported-models.txt" || true
fi

PY="$COMFY_ROOT/.venv/bin/python"
if [[ ! -x "$PY" ]]; then
    echo "ERROR: ComfyUI venv python not found: $PY" >&2
    exit 9
fi

(
    cd "$COMFY_ROOT"
    PYTHONPATH="$COMFY_ROOT" "$PY" - <<'PY'
import json
info = {}
try:
    import comfy
    info["comfy_import"] = "PASS"
except Exception as exc:
    info["comfy_import"] = f"FAIL: {exc}"

try:
    import folder_paths
    info["folder_paths_import"] = "PASS"
except Exception as exc:
    info["folder_paths_import"] = f"FAIL: {exc}"

try:
    import torch
    info["torch"] = torch.__version__
    info["torch_cuda"] = torch.version.cuda
    info["cuda_available"] = torch.cuda.is_available()
    if torch.cuda.is_available():
        info["device_name_0"] = torch.cuda.get_device_name(0)
        info["device_total_memory"] = int(torch.cuda.get_device_properties(0).total_memory)
except Exception as exc:
    info["torch_error"] = str(exc)

print(json.dumps(info, indent=2, sort_keys=True))
if info.get("comfy_import") != "PASS" or info.get("folder_paths_import") != "PASS":
    raise SystemExit(10)
PY
) | tee "$OUT/comfy-import.txt"

echo
echo "=== COMFYUI MODEL VISIBILITY ==="
python3 - "$COMFY_ROOT" "$DIFFUSION" "$ENCODER" "$VAE" "$OUT/comfy-model-visibility.json" <<'PY'
import json
import pathlib
import sys

comfy = pathlib.Path(sys.argv[1])
files = {
    "diffusion_models": pathlib.Path(sys.argv[2]),
    "text_encoders": pathlib.Path(sys.argv[3]),
    "vae": pathlib.Path(sys.argv[4]),
}
expected_names = {
    "diffusion_models": files["diffusion_models"].name,
    "text_encoders": files["text_encoders"].name,
    "vae": files["vae"].name,
}

search_dirs = {
    "diffusion_models": [
        comfy / "models" / "diffusion_models",
        comfy / "models" / "unet",
    ],
    "text_encoders": [
        comfy / "models" / "text_encoders",
        comfy / "models" / "clip",
    ],
    "vae": [
        comfy / "models" / "vae",
    ],
}

rows = {}
for category, name in expected_names.items():
    matches = []
    for root in search_dirs[category]:
        if not root.exists():
            continue
        for path in root.rglob(name):
            matches.append(str(path))
    rows[category] = {
        "expected_name": name,
        "source_path": str(files[category]),
        "visible_matches": sorted(set(matches)),
        "visible": bool(matches),
    }

out = pathlib.Path(sys.argv[5])
out.write_text(json.dumps(rows, indent=2, sort_keys=True) + "\n")

for category, row in rows.items():
    state = "VISIBLE" if row["visible"] else "NOT_STAGED"
    print(f"{category}: {state}")
    for match in row["visible_matches"]:
        print(f"  {match}")
PY

echo
echo "=== MACHINE SNAPSHOT ==="
{
    free -h
    echo
    nvidia-smi
} | tee "$OUT/machine.txt"

echo
echo "=== SELECTION RESULT ==="
cat <<EOF | tee "$OUT/selection.txt"
selected_candidate=FLUX.2-Klein-4B
selected_variant=distilled-fp8
text_encoder=qwen_3_4b_fp4_flux2.safetensors
vae=flux2-vae.safetensors
selection_status=PROVISIONAL_QUALIFIED
selection_reason=lower-risk reproducible external oracle with native ComfyUI Flux2 support, explicit multi-component resources, iterative latent/scheduler semantics, and sufficient structural distance from Qwen2
deferred_candidate=Qwen-Image-2.1
deferred_reason=retain as later higher-pressure residency/offload discriminator; installed package is substantially larger than device VRAM budget when components are considered together
EOF

echo
echo "=== EVIDENCE CHECKSUMS ==="
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
with (root / "SHA256SUMS.txt").open("w") as fh:
    for path in sorted(p for p in root.rglob("*") if p.is_file() and p.name != "SHA256SUMS.txt"):
        fh.write(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(root)}\n")
PY

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 7A selection preflight mutated AIR source worktree." >&2
    git status --short >&2
    exit 11
fi

echo
echo "PROMPT7A_FLUX2_SELECTION_PREFLIGHT=PASS"
echo "Evidence directory: $OUT"
