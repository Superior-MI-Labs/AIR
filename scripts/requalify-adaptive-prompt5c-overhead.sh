#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18560}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P5C_OVERHEAD_OUT:-$HOME/Downloads/AIR-0.11-Prompt5C-Overhead-$STAMP}"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [base-port]" >&2
    exit 2
fi

mkdir -p "$OUT"
cd "$ROOT"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 5C overhead remeasurement requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

AIR_P3_OVERHEAD_OUT="$OUT" \
    bash "$ROOT/scripts/requalify-adaptive-prompt3-overhead.sh" \
    "$MODEL" "$BASE_PORT"

python3 - "$OUT/observer-overhead-v2.json" <<'PY'
import json
import pathlib
import sys

summary = json.loads(pathlib.Path(sys.argv[1]).read_text())
detailed = summary["detailed"]
normal = summary["normal"]
off = summary["off"]

assert detailed["samples"] == 18
assert normal["samples"] == 18
assert off["samples"] == 18

print("PROMPT5C_OVERHEAD_REMEASURE=PASS")
print(
    "detailed_delta_percent_vs_normal="
    f"{detailed['median_session_delta_percent_vs_normal']:.3f}"
)
print(
    "detailed_delta_percent_vs_off="
    f"{detailed['median_session_delta_percent_vs_off']:.3f}"
)
print(
    "normal_delta_percent_vs_off="
    f"{normal['median_session_delta_percent_vs_off']:.3f}"
)
PY

echo "Evidence directory: $OUT"
