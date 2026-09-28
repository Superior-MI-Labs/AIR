#!/usr/bin/env bash
set -euo pipefail

MODEL="${1:-}"
PORT="${2:-18360}"
if [[ -z "$MODEL" || ! -f "$MODEL" ]]; then
    echo "Usage: $0 /path/to/model.gguf [port]" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BRANCH="$(git branch --show-current)"
HEAD="$(git rev-parse HEAD)"
if [[ "$BRANCH" != "architecture/modular-runtime-r0" ]]; then
    echo "ERROR: Wave 7 qualification requires architecture/modular-runtime-r0; current=$BRANCH" >&2
    exit 2
fi
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Wave 7 qualification requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_W7_OUT:-$HOME/Downloads/AIR-Modular-Runtime-W7-$STAMP}"
BUILD_DIR="$OUT/build"
PREFIX="$OUT/prefix"
RC_OUT="$OUT/rc"
RC_ARCHIVE="$OUT/rc.zip"
mkdir -p "$OUT"

{
    echo "date=$(date -Is)"
    echo "branch=$BRANCH"
    echo "head=$HEAD"
    echo "model=$MODEL"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
    echo "model_bytes=$(stat -c %s "$MODEL")"
    echo "build_dir=$BUILD_DIR"
    echo "prefix=$PREFIX"
} > "$OUT/identity.txt"

AIR_ENABLE_CUDA=ON \
BUILD_DIR="$BUILD_DIR" \
PREFIX="$PREFIX" \
"$ROOT/scripts/install-local.sh" 2>&1 | \
    tee "$OUT/install-and-ctest.txt"


# Produce the current report once, then build frozen v0.9.12 on this exact
# machine/toolchain and retain its verification JSON as the numerical baseline.
PRECHECK_JSON="$OUT/precheck-verification.json"
set +e
"$PREFIX/bin/air-verify" -m "$MODEL" \
    --prompt "The capital of France is" \
    --generate 16 --top-k 8 --atol 0.001 --device 0 \
    --output "$PRECHECK_JSON" \
    > "$OUT/precheck-verification.txt" 2>&1
PRECHECK_RC="$?"
set -e
echo "precheck_strict_verify_exit=$PRECHECK_RC" >> "$OUT/identity.txt"

BASELINE_OUT="$OUT/v0912-baseline"
AIR_VERIFY_BASELINE_OUT="$BASELINE_OUT" \
"$ROOT/scripts/compare-v0912-verification.sh" \
    "$MODEL" "$PRECHECK_JSON" 2>&1 | \
    tee "$OUT/v0912-baseline-comparison.txt"

BASELINE_JSON="$BASELINE_OUT/v0.9.12-verification.json"
if [[ ! -f "$BASELINE_JSON" ]]; then
    echo "ERROR: frozen v0.9.12 verification baseline was not produced." >&2
    exit 1
fi
echo "verification_baseline_json=$BASELINE_JSON" >> "$OUT/identity.txt"
echo "verification_baseline_sha256=$(sha256sum "$BASELINE_JSON" | awk '{print $1}')" >> "$OUT/identity.txt"

PATH="$PREFIX/bin:$PATH" \
AIR_PREFIX="$PREFIX" \
AIR_BIN_DIR="$PREFIX/bin" \
AIR_RC_OUT="$RC_OUT" \
AIR_RC_ARCHIVE="$RC_ARCHIVE" \
AIR_VERIFICATION_BASELINE_JSON="$BASELINE_JSON" \
"$ROOT/scripts/validate-rc-machine.sh" "$MODEL" "$PORT" 2>&1 | \
    tee "$OUT/validate-rc.txt"

{
    cat "$OUT/identity.txt"
    echo "rc_archive=$RC_ARCHIVE"
    echo "rc_archive_sha256=$(sha256sum "$RC_ARCHIVE" | awk '{print $1}')"
    echo
    echo "=== release-candidate gates ==="
    cat "$RC_OUT/gates.txt"
    echo
    echo "=== release-candidate summary ==="
    cat "$RC_OUT/summary.txt"
} > "$OUT/wave7-summary.txt"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: qualification mutated the source worktree." >&2
    git status --short >&2
    exit 1
fi

echo
echo "===== AIR MODULAR RUNTIME WAVE 7 ====="
cat "$OUT/wave7-summary.txt"
echo
echo "Evidence directory:"
echo "$OUT"
