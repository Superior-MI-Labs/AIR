#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
PORT="${2:-18821}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P8B_OUT:-$HOME/Downloads/AIR-0.11-Prompt8B-Resources-$STAMP}"
BUILD="$OUT/build-cuda"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [port]" >&2
    exit 2
fi
if [[ ! -f "$MODEL" ]]; then
    echo "ERROR: model not found: $MODEL" >&2
    exit 2
fi
if ! command -v nvidia-smi >/dev/null 2>&1; then
    echo "ERROR: nvidia-smi is required for Prompt 8B CUDA qualification" >&2
    exit 2
fi

mkdir -p "$OUT"
cd "$ROOT"

SERVER_PID=""
STAGE="initialization"

cleanup() {
    if [[ -n "$SERVER_PID" ]]; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}

on_error() {
    local rc=$?
    cleanup
    echo >&2
    echo "PROMPT8B_PREPARED_RESOURCE_RESIDENCY=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in         "$OUT/preflight-terminal.txt"         "$OUT/cmake-cuda.txt"         "$OUT/build-cuda.txt"         "$OUT/ctest-cuda.txt"         "$OUT/cuda-contract.txt"         "$OUT/server.log"         "$OUT/generation.json"         "$OUT/runtime.json"; do
        if [[ -s "$log" ]]; then
            echo >&2
            echo "=== tail: $(basename "$log") ===" >&2
            tail -n 180 "$log" >&2
        fi
    done
    exit "$rc"
}
trap on_error ERR
trap cleanup EXIT

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 8B qualification requires a clean AIR worktree." >&2
    git status --short >&2
    exit 2
fi

if [[ "$(git branch --show-current)" != "architecture/adaptive-execution-substrate-r0" ]]; then
    echo "ERROR: Prompt 8B qualifier must run on architecture/adaptive-execution-substrate-r0" >&2
    exit 2
fi

STAGE="gpu-baseline"
compute_apps="$(
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader 2>/dev/null         | sed '/^[[:space:]]*$/d' || true
)"
printf '%s\n' "$compute_apps" > "$OUT/gpu-compute-baseline.txt"
if [[ -n "$compute_apps" ]]; then
    echo "ERROR: GPU compute process already active; Prompt 8B requires a clean compute baseline" >&2
    cat "$OUT/gpu-compute-baseline.txt" >&2
    exit 3
fi

{
    echo "date=$(date -Is)"
    echo "branch=$(git branch --show-current)"
    echo "head=$(git rev-parse HEAD)"
    echo "host=$(hostname)"
    echo "kernel=$(uname -srmo)"
    echo "model=$MODEL"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
} > "$OUT/identity.txt"

echo "=== ADAPTIVE CPU PREFLIGHT ==="
STAGE="cpu-preflight"
AIR_PREFLIGHT_OUT="$OUT/preflight-build"     bash "$ROOT/scripts/preflight-adaptive.sh"     2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== FRESH CUDA BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=ON     > "$OUT/cmake-cuda.txt" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" -j"$(nproc)"     > "$OUT/build-cuda.txt" 2>&1

echo
echo "=== FULL CUDA CTEST ==="
STAGE="cuda-ctest"
ctest --test-dir "$BUILD" --output-on-failure     | tee "$OUT/ctest-cuda.txt"

echo
echo "=== IDENTIFIED PREPARED-RESOURCE CUDA CONTRACT ==="
STAGE="prepared-resource-contract"
"$BUILD/air-cuda-contract-tests"     2>&1 | tee "$OUT/cuda-contract.txt"

grep -Fq "CUDA prepared backend operation-site legality passed"     "$OUT/cuda-contract.txt"
grep -Fq "CUDA identified prepared-resource residency transition passed"     "$OUT/cuda-contract.txt"
grep -Fq "CUDA execution and dense tactic are independent of Qwen2 source names"     "$OUT/cuda-contract.txt"

wait_health() {
    for _ in $(seq 1 160); do
        if curl -fsS "http://127.0.0.1:$PORT/health" >/dev/null 2>&1; then
            return 0
        fi
        if [[ -s "$OUT/server.log" ]] &&
           grep -Eq 'failed|error|ERROR|fatal|FATAL' "$OUT/server.log"; then
            tail -n 120 "$OUT/server.log" >&2
        fi
        sleep 0.25
    done
    echo "server did not become healthy on port $PORT" >&2
    return 1
}

echo
echo "=== REAL QWEN CUDA NONREGRESSION ==="
STAGE="server-start"
"$BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$PORT"     --no-manifest     --execution-observation normal     > "$OUT/server.log" 2>&1 &
SERVER_PID=$!
wait_health

STAGE="server-generate"
curl -fsS --max-time 120     -H 'Content-Type: application/json'     -d '{"prompt":"AIR Prompt 8B prepared-resource qualification.","max_tokens":4,"temperature":0.0}'     "http://127.0.0.1:$PORT/generate"     > "$OUT/generation.json"

curl -fsS "http://127.0.0.1:$PORT/runtime"     > "$OUT/runtime.json"

STAGE="server-validation"
python3 - "$OUT/generation.json" "$OUT/runtime.json" <<'PY'
import json
import pathlib
import sys

generation = json.loads(pathlib.Path(sys.argv[1]).read_text())
runtime = json.loads(pathlib.Path(sys.argv[2]).read_text())

metrics = generation["metrics"]
assert metrics["backend"] == "cuda"
assert int(metrics["generated_tokens"]) > 0
planner = runtime["planner"]
assert planner["selected_backend"] == "cuda"

print("prompt8b_real_qwen_nonregression=PASS")
print("selected_backend=" + planner["selected_backend"])
print("generated_tokens=" + str(metrics["generated_tokens"]))
PY

cleanup

STAGE="gpu-cleanup"
sleep 1
remaining="$(
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader 2>/dev/null         | sed '/^[[:space:]]*$/d' || true
)"
printf '%s\n' "$remaining" > "$OUT/gpu-compute-final.txt"
if [[ -n "$remaining" ]]; then
    echo "ERROR: Prompt 8B left a GPU compute process alive" >&2
    cat "$OUT/gpu-compute-final.txt" >&2
    exit 4
fi

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 8B qualification mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

nvidia-smi     --query-gpu=index,name,uuid,memory.total,memory.free,driver_version,pstate,temperature.gpu,power.draw,clocks.sm,clocks.mem     --format=csv,noheader     > "$OUT/nvidia-smi.txt" || true

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
out = root / "SHA256SUMS.txt"
with out.open("w") as handle:
    for path in sorted(p for p in root.rglob("*") if p.is_file() and p != out):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        handle.write(f"{digest}  {path.relative_to(root)}\n")
print(f"checksums={out}")
PY

trap - ERR
trap - EXIT
cleanup

echo
echo "PROMPT8B_PREPARED_RESOURCE_RESIDENCY=PASS"
echo "Evidence directory: $OUT"
