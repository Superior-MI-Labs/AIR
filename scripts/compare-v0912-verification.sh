#!/usr/bin/env bash
set -euo pipefail

MODEL="${1:-}"
CURRENT_JSON="${2:-}"
if [[ -z "$MODEL" || ! -f "$MODEL" || -z "$CURRENT_JSON" || ! -f "$CURRENT_JSON" ]]; then
    echo "Usage: $0 /path/to/model.gguf /path/to/current/verification.json" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_VERIFY_BASELINE_OUT:-$HOME/Downloads/AIR-v0.9.12-Verification-Baseline-$STAMP}"
WORKTREE="$OUT/source-v0.9.12"
BUILD="$OUT/build-v0.9.12"
TAG_JSON="$OUT/v0.9.12-verification.json"
TAG_LOG="$OUT/v0.9.12-verification.txt"
mkdir -p "$OUT"

cleanup() {
    git -C "$ROOT" worktree remove --force "$WORKTREE" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

git fetch --tags origin >/dev/null
git worktree add --detach "$WORKTREE" v0.9.12 >/dev/null

cmake -S "$WORKTREE" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAIR_ENABLE_CUDA=ON
cmake --build "$BUILD" --target air-verify -j"${JOBS:-$(nproc)}"

set +e
"$BUILD/air-verify" -m "$MODEL" \
    --prompt "The capital of France is" \
    --generate 16 --top-k 8 --atol 0.001 --device 0 \
    --output "$TAG_JSON" \
    > "$TAG_LOG" 2>&1
TAG_RC=$?
set -e

python3 - "$CURRENT_JSON" "$TAG_JSON" "$TAG_RC" <<'PY' | tee "$OUT/comparison.txt"
import json
import pathlib
import statistics
import sys

current_path = pathlib.Path(sys.argv[1])
tag_path = pathlib.Path(sys.argv[2])
tag_rc = int(sys.argv[3])

current = json.loads(current_path.read_text())
tag = json.loads(tag_path.read_text())

def summarize(doc):
    decisions = doc.get("decisions", [])
    errors = [float(d.get("logits", {}).get("max_abs_error", 0.0)) for d in decisions]
    rms = [float(d.get("logits", {}).get("rms_error", 0.0)) for d in decisions]
    return {
        "schema": doc.get("schema"),
        "air_version": doc.get("air_version"),
        "decisions": len(decisions),
        "top1_parity": bool(doc.get("summary", {}).get("top1_parity")),
        "finite": bool(doc.get("summary", {}).get("finite")),
        "within_requested_atol": bool(doc.get("summary", {}).get("within_requested_atol")),
        "max_abs_error": max(errors, default=0.0),
        "mean_decision_max_abs_error": statistics.mean(errors) if errors else 0.0,
        "max_rms_error": max(rms, default=0.0),
    }

c = summarize(current)
t = summarize(tag)

print("AIR verification baseline comparison")
print("current_json=", current_path, sep="")
print("tag_json=", tag_path, sep="")
print("tag_exit_code=", tag_rc, sep="")
print()
for label, value in (("current", c), ("v0.9.12", t)):
    print(f"[{label}]")
    for key, item in value.items():
        print(f"{key}={item}")
    print()

if not t["top1_parity"] or not t["finite"]:
    verdict = "BASELINE_NUMERICAL_FAILURE"
elif t["within_requested_atol"] and not c["within_requested_atol"]:
    verdict = "BRANCH_REGRESSION_SUSPECTED"
elif not t["within_requested_atol"] and not c["within_requested_atol"]:
    ratio = c["max_abs_error"] / t["max_abs_error"] if t["max_abs_error"] else float("inf")
    print("current_to_tag_max_error_ratio=", ratio, sep="")
    verdict = "STRICT_ATOL_NOT_REPRODUCIBLE_BASELINE_GATE_REQUIRED"
else:
    verdict = "CURRENT_GATE_PASSES_OR_INCONCLUSIVE"

print("verdict=", verdict, sep="")
PY

set +e
python3 "$ROOT/scripts/compare-verification-baseline.py" \
    --current "$CURRENT_JSON" \
    --baseline "$TAG_JSON" \
    --output "$OUT/baseline-gate.json" \
    2>&1 | tee "$OUT/baseline-gate.txt"
BASELINE_GATE_RC="${PIPESTATUS[0]}"
set -e

{
    echo "model=$MODEL"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
    echo "current_json=$CURRENT_JSON"
    echo "current_json_sha256=$(sha256sum "$CURRENT_JSON" | awk '{print $1}')"
    echo "tag=v0.9.12"
    echo "tag_commit=$(git -C "$WORKTREE" rev-parse HEAD)"
    echo "tag_verify_exit=$TAG_RC"
    echo "baseline_gate_exit=$BASELINE_GATE_RC"
    echo "nvcc=$(nvcc --version | tail -n1)"
    nvidia-smi --query-gpu=name,driver_version --format=csv,noheader
} > "$OUT/identity.txt"

echo
cat "$OUT/comparison.txt"
echo
echo "Evidence directory:"
echo "$OUT"
echo
echo "baseline_gate_exit=$BASELINE_GATE_RC"
exit "$BASELINE_GATE_RC"
