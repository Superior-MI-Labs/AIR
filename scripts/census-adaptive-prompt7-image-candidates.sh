#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE_ROOT="${1:-$HOME/Models/Media/Image}"
COMFY_ROOT="${2:-$HOME/Projects/AI-Runtimes/ComfyUI}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P7A_OUT:-$HOME/Downloads/AIR-0.11-Prompt7A-Image-Candidates-$STAMP}"

mkdir -p "$OUT"
cd "$ROOT"

echo "=== PROMPT 7A IMAGE WORKFLOW CANDIDATE CENSUS ==="
echo "output=$OUT"
echo

{
    echo "date=$(date -Is)"
    echo "air_branch=$(git branch --show-current)"
    echo "air_head=$(git rev-parse HEAD)"
    echo "host=$(hostname)"
    echo "kernel=$(uname -srmo)"
    echo "image_root=$IMAGE_ROOT"
    echo "comfy_root=$COMFY_ROOT"
} > "$OUT/identity.txt"

echo "=== MACHINE ==="
{
    free -h
    echo
    df -h "$HOME" || true
    echo
    if command -v nvidia-smi >/dev/null; then
        nvidia-smi
    fi
} | tee "$OUT/machine.txt"

echo
echo "=== IMAGE PACKAGE ROOT ==="
if [[ ! -d "$IMAGE_ROOT" ]]; then
    echo "ERROR: image model root not found: $IMAGE_ROOT" >&2
    exit 2
fi

find "$IMAGE_ROOT" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' \
    | sort | tee "$OUT/candidate-directories.txt"

echo
echo "=== CANDIDATE DIRECTORY SIZES ==="
while IFS= read -r name; do
    [[ -n "$name" ]] || continue
    du -sh "$IMAGE_ROOT/$name"
done < "$OUT/candidate-directories.txt" \
    | tee "$OUT/candidate-sizes.txt"

echo
echo "=== CANDIDATE STRUCTURE ==="
python3 - "$IMAGE_ROOT" "$OUT/candidate-structure.json" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])

interesting_names = {
    "model_index.json",
    "config.json",
    "scheduler_config.json",
    "tokenizer_config.json",
    "tokenizer.json",
    "special_tokens_map.json",
    "preprocessor_config.json",
    "generation_config.json",
    "vae_config.json",
    "README.md",
    ".gitattributes",
}

interesting_suffixes = {
    ".json",
    ".yaml",
    ".yml",
    ".toml",
    ".txt",
}

rows = []
for candidate in sorted(p for p in root.iterdir() if p.is_dir()):
    files = []
    large = []
    for path in candidate.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(candidate)
        depth = len(rel.parts)
        try:
            size = path.stat().st_size
        except OSError:
            size = None

        if depth <= 4 and (
            path.name in interesting_names
            or path.suffix.lower() in interesting_suffixes
            or "config" in path.name.lower()
            or "scheduler" in path.name.lower()
            or "tokenizer" in path.name.lower()
            or "vae" in path.name.lower()
            or "text_encoder" in str(rel).lower()
        ):
            files.append({"path": str(rel), "bytes": size})

        if size is not None and size >= 100 * 1024 * 1024:
            large.append(
                {
                    "path": str(rel),
                    "bytes": size,
                    "gib": size / (1024**3),
                }
            )

    rows.append(
        {
            "candidate": candidate.name,
            "path": str(candidate),
            "metadata_files": sorted(files, key=lambda x: x["path"]),
            "large_files": sorted(large, key=lambda x: (-x["bytes"], x["path"])),
        }
    )

out.write_text(json.dumps(rows, indent=2, sort_keys=True) + "\n")

for row in rows:
    print(f"\n--- {row['candidate']} ---")
    print(f"path={row['path']}")
    print("metadata/config:")
    for item in row["metadata_files"][:80]:
        print(f"  {item['path']} ({item['bytes']} bytes)")
    print("large artifacts:")
    for item in row["large_files"][:80]:
        print(f"  {item['path']} ({item['gib']:.3f} GiB)")
PY

echo
echo "=== SAFE CONFIG EXCERPTS ==="
python3 - "$IMAGE_ROOT" "$OUT/config-excerpts.json" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])

preferred = (
    "model_index.json",
    "config.json",
    "scheduler_config.json",
    "tokenizer_config.json",
    "preprocessor_config.json",
)

records = []
for candidate in sorted(p for p in root.iterdir() if p.is_dir()):
    for path in candidate.rglob("*"):
        if not path.is_file() or path.name not in preferred:
            continue
        if len(path.relative_to(candidate).parts) > 4:
            continue
        try:
            raw = path.read_text(errors="replace")
        except Exception as exc:
            records.append(
                {
                    "candidate": candidate.name,
                    "path": str(path.relative_to(candidate)),
                    "error": str(exc),
                }
            )
            continue

        records.append(
            {
                "candidate": candidate.name,
                "path": str(path.relative_to(candidate)),
                "text": raw[:20000],
                "truncated": len(raw) > 20000,
            }
        )

out.write_text(json.dumps(records, indent=2, sort_keys=True) + "\n")
print(f"retained_config_records={len(records)}")
PY

echo
echo "=== COMFYUI ORACLE RUNTIME ==="
if [[ -d "$COMFY_ROOT" ]]; then
    {
        echo "comfy_root=$COMFY_ROOT"
        if git -C "$COMFY_ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            echo "git_head=$(git -C "$COMFY_ROOT" rev-parse HEAD)"
            echo "git_branch=$(git -C "$COMFY_ROOT" branch --show-current)"
            echo "git_status_begin"
            git -C "$COMFY_ROOT" status --short
            echo "git_status_end"
        fi

        PY="$COMFY_ROOT/.venv/bin/python"
        if [[ ! -x "$PY" ]]; then
            PY="$(command -v python3)"
        fi
        echo "python=$PY"
        "$PY" --version 2>&1 || true

        "$PY" - <<'PY' 2>&1 || true
import json
info = {}
try:
    import torch
    info["torch"] = torch.__version__
    info["torch_cuda"] = torch.version.cuda
    info["cuda_available"] = torch.cuda.is_available()
    if torch.cuda.is_available():
        info["device_count"] = torch.cuda.device_count()
        info["device_name_0"] = torch.cuda.get_device_name(0)
        p = torch.cuda.get_device_properties(0)
        info["device_total_memory"] = int(p.total_memory)
except Exception as exc:
    info["torch_error"] = str(exc)

for name in ("comfy", "diffusers", "transformers", "safetensors", "accelerate"):
    try:
        module = __import__(name)
        info[name] = getattr(module, "__version__", "present")
    except Exception as exc:
        info[name] = f"unavailable: {exc}"

print(json.dumps(info, indent=2, sort_keys=True))
PY
    } | tee "$OUT/comfy-runtime.txt"

    echo
    echo "=== COMFYUI CUSTOM NODES ==="
    if [[ -d "$COMFY_ROOT/custom_nodes" ]]; then
        find "$COMFY_ROOT/custom_nodes" -mindepth 1 -maxdepth 1 \
            -printf '%f\n' | sort | tee "$OUT/comfy-custom-nodes.txt"
    else
        : > "$OUT/comfy-custom-nodes.txt"
    fi

    echo
    echo "=== COMFYUI MODEL SEARCH PATHS ==="
    {
        if [[ -f "$COMFY_ROOT/extra_model_paths.yaml" ]]; then
            cat "$COMFY_ROOT/extra_model_paths.yaml"
        elif [[ -f "$COMFY_ROOT/extra_model_paths.yaml.example" ]]; then
            echo "(example only)"
            cat "$COMFY_ROOT/extra_model_paths.yaml.example"
        else
            echo "No extra_model_paths.yaml found."
        fi
    } > "$OUT/comfy-model-paths.txt"
    cat "$OUT/comfy-model-paths.txt"
else
    echo "ComfyUI root not found: $COMFY_ROOT" | tee "$OUT/comfy-runtime.txt"
    : > "$OUT/comfy-custom-nodes.txt"
    : > "$OUT/comfy-model-paths.txt"
fi

echo
echo "=== CANDIDATE SELECTION CHECKLIST ==="
cat > "$OUT/selection-checklist.txt" <<'EOF'
For each candidate, Prompt 7A must establish:

[ ] runs externally on WolfCat
[ ] exact runtime/version can be frozen
[ ] exact workflow graph/config can be retained
[ ] fixed-seed run can be repeated
[ ] dimensions / step count / scheduler / seed are explicit
[ ] output artifact can be retained
[ ] component/resource structure is inspectable
[ ] peak VRAM and host RAM can be measured
[ ] residency/offload behavior is observable enough for Prompt 7
[ ] workflow is structurally different enough from Qwen2
[ ] no AIR package-supplied arbitrary code execution is required
EOF
cat "$OUT/selection-checklist.txt"

echo
echo "=== CHECKSUMS ==="
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
files = [p for p in root.rglob("*") if p.is_file() and p.name != "SHA256SUMS.txt"]
with (root / "SHA256SUMS.txt").open("w") as fh:
    for path in sorted(files):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        fh.write(f"{digest}  {path.relative_to(root)}\n")
PY

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 7A census mutated AIR source worktree." >&2
    git status --short >&2
    exit 4
fi

echo
echo "PROMPT7A_IMAGE_CANDIDATE_CENSUS=PASS"
echo "Evidence directory: $OUT"
