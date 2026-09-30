#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: $0 /path/to/model.gguf [prompt6b-evidence-dir]" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODEL="$1"
if [[ $# -ge 2 ]]; then
    P6B="$2"
else
    P6B="$HOME/Downloads/AIR-0.11-Prompt6B-Prefill-Tactics-20260930-050028"
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="$HOME/Downloads/AIR-0.11-Prompt6C-Transitions-$STAMP"
BUILD="$OUT/build-cuda"
STAGE="initialization"

mkdir -p "$OUT"
cd "$ROOT"

on_error() {
    rc=$?
    set +e
    echo
    echo "PROMPT6C_TRANSITION_ECONOMICS=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in \
        "$OUT/preflight-terminal.txt" \
        "$OUT/configure.log" \
        "$OUT/build.log" \
        "$OUT/ctest-cuda.txt" \
        "$OUT/prompt6c-terminal.txt"; do
        if [[ -s "$log" ]]; then
            echo >&2
            echo "=== tail: $(basename "$log") ===" >&2
            tail -n 180 "$log" >&2
        fi
    done
    exit "$rc"
}
trap on_error ERR

if [[ ! -f "$P6B/prompt6b-prefill-tactic-summary.json" ]]; then
    echo "ERROR: missing Prompt 6B evidence directory: $P6B" >&2
    exit 2
fi

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 6C requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

{
    echo "date=$(date -Is)"
    echo "branch=$(git branch --show-current)"
    echo "head=$(git rev-parse HEAD)"
    echo "host=$(hostname)"
    echo "kernel=$(uname -srmo)"
    echo "model=$MODEL"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
    echo "prompt6b_evidence=$P6B"
} > "$OUT/identity.txt"

if command -v nvidia-smi >/dev/null; then
    nvidia-smi > "$OUT/nvidia-smi-before.txt" || true
    nvidia-smi \
        --query-compute-apps=pid,process_name,used_memory \
        --format=csv,noheader \
        > "$OUT/compute-apps-before.txt" 2>/dev/null || true
fi

echo "=== ADAPTIVE CPU PREFLIGHT ==="
STAGE="cpu-preflight"
AIR_PREFLIGHT_OUT="$OUT/preflight-build" \
    bash "$ROOT/scripts/preflight-adaptive.sh" \
    2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== CUDA RESEARCH BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAIR_ENABLE_CUDA=ON \
    -DAIR_BUILD_RESEARCH=ON \
    > "$OUT/configure.log" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" -j"$(nproc)" \
    > "$OUT/build.log" 2>&1

echo
echo "=== CUDA CTEST ==="
STAGE="cuda-ctest"
ctest --test-dir "$BUILD" --output-on-failure \
    | tee "$OUT/ctest-cuda.txt"

echo
echo "=== PROMPT 6C RESIDENCY / TRANSITION ECONOMICS ==="
STAGE="transition-economics"
python3 "$ROOT/scripts/prompt6c_transition_probe.py" \
    --model "$MODEL" \
    --p6b "$P6B" \
    --out "$OUT" \
    --build "$BUILD" \
    2>&1 | tee "$OUT/prompt6c-terminal.txt"

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 6C mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

if command -v nvidia-smi >/dev/null; then
    nvidia-smi > "$OUT/nvidia-smi-after.txt" || true
    nvidia-smi \
        --query-compute-apps=pid,process_name,used_memory \
        --format=csv,noheader \
        > "$OUT/compute-apps-after.txt" 2>/dev/null || true
fi

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root=pathlib.Path(sys.argv[1])
files=[p for p in root.rglob("*") if p.is_file() and p.name!="SHA256SUMS.txt"]
with (root/"SHA256SUMS.txt").open("w") as out:
    for p in sorted(files):
        out.write(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(root)}\n")
PY

trap - ERR

echo
echo "PROMPT6C_TRANSITION_ECONOMICS=PASS"
echo "Evidence directory: $OUT"
