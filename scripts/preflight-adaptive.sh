#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${AIR_PREFLIGHT_OUT:-$ROOT/build-adaptive-preflight}"
STAGE="initialization"

on_error() {
    local rc=$?
    set +e
    echo >&2
    echo "ADAPTIVE_PREFLIGHT=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "build_dir=$OUT" >&2
    if [[ -f "$OUT/configure.log" ]]; then
        echo >&2
        echo "=== tail: configure.log ===" >&2
        tail -n 120 "$OUT/configure.log" >&2
    fi
    if [[ -f "$OUT/build.log" ]]; then
        echo >&2
        echo "=== tail: build.log ===" >&2
        tail -n 160 "$OUT/build.log" >&2
    fi
    if [[ -f "$OUT/ctest.log" ]]; then
        echo >&2
        echo "=== tail: ctest.log ===" >&2
        tail -n 160 "$OUT/ctest.log" >&2
    fi
    exit "$rc"
}
trap on_error ERR

cd "$ROOT"

STAGE="shell-syntax"
while IFS= read -r script; do
    bash -n "$script"
done < <(find scripts -maxdepth 1 -type f -name '*.sh' -print | sort)

STAGE="python-syntax"
while IFS= read -r script; do
    python3 - "$script" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
compile(path.read_text(), str(path), "exec")
PY
done < <(find scripts -maxdepth 1 -type f -name '*.py' -print | sort)

STAGE="clean-build-dir"
rm -rf "$OUT"
mkdir -p "$OUT"

STAGE="cpu-configure"
cmake -S "$ROOT" -B "$OUT"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=OFF     > "$OUT/configure.log" 2>&1

STAGE="cpu-build"
cmake --build "$OUT" -j"${AIR_BUILD_JOBS:-$(nproc)}"     > "$OUT/build.log" 2>&1

STAGE="cpu-ctest"
ctest --test-dir "$OUT" --output-on-failure     | tee "$OUT/ctest.log"

trap - ERR

echo
echo "ADAPTIVE_PREFLIGHT=PASS"
echo "build_dir=$OUT"
