#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ORACLE_DIR="${1:-$HOME/Downloads/AIR-0.11-Prompt7B-FLUX2-Oracle-20260930-145158}"
COMFY_ROOT="${2:-$HOME/Projects/AI-Runtimes/ComfyUI}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P7CH_OUT:-$HOME/Downloads/AIR-0.11-Prompt7C-H-FLUX2-Census-$STAMP}"

cd "$ROOT"

echo "=== PROMPT 7C-H FLUX.2 ORACLE ANALYSIS ==="
echo "oracle=$ORACLE_DIR"
echo "comfy=$COMFY_ROOT"
echo "output=$OUT"
echo

[[ -z "$(git status --porcelain)" ]] || {
    echo "ERROR: Prompt 7C-H analysis requires a clean AIR worktree." >&2
    git status --short >&2
    exit 2
}

[[ -d "$ORACLE_DIR" ]] || {
    echo "ERROR: qualified Prompt 7B evidence directory not found: $ORACLE_DIR" >&2
    exit 3
}

[[ -f "$ORACLE_DIR/oracle-summary.json" ]] || {
    echo "ERROR: Prompt 7B oracle summary missing." >&2
    exit 4
}

[[ -d "$COMFY_ROOT/.git" ]] || {
    echo "ERROR: pinned ComfyUI checkout missing: $COMFY_ROOT" >&2
    exit 5
}

mkdir -p "$OUT"

python3 scripts/analyze-adaptive-prompt7c-h-flux2.py \
    "$ORACLE_DIR" \
    "$COMFY_ROOT" \
    "$ROOT" \
    "$OUT" \
    2>&1 | tee "$OUT/terminal.txt"

rc=${PIPESTATUS[0]}

echo
echo "prompt7c_h_rc=$rc"

if [[ "$rc" -ne 0 ]]; then
    exit "$rc"
fi

[[ -z "$(git status --porcelain)" ]] || {
    echo "ERROR: Prompt 7C-H analysis mutated AIR source worktree." >&2
    git status --short >&2
    exit 6
}
