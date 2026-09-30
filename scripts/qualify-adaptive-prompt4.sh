#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
PORT="${2:-18520}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P4_OUT:-$HOME/Downloads/AIR-0.11-Prompt4-$STAMP}"
BUILD="$OUT/build-cuda"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [port]" >&2
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
    echo "PROMPT4_OPERATION_BOUNDARY=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in         "$OUT/preflight-terminal.txt"         "$OUT/cmake-cuda.txt"         "$OUT/build-cuda.txt"         "$OUT/ctest-cuda.txt"         "$OUT/cuda-contract.txt"         "$OUT/server.log"; do
        if [[ -s "$log" ]]; then
            echo >&2
            echo "=== tail: $(basename "$log") ===" >&2
            tail -n 160 "$log" >&2
        fi
    done
    exit "$rc"
}
trap on_error ERR
trap cleanup EXIT

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 4 qualification requires a clean worktree." >&2
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
} > "$OUT/identity.txt"

echo "=== ADAPTIVE CPU PREFLIGHT ==="
STAGE="cpu-preflight"
AIR_PREFLIGHT_OUT="$OUT/preflight-build"     bash "$ROOT/scripts/preflight-adaptive.sh"     2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== CUDA BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=ON     > "$OUT/cmake-cuda.txt" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" -j"$(nproc)"     > "$OUT/build-cuda.txt" 2>&1

echo
echo "=== CUDA CTEST ==="
STAGE="cuda-ctest"
ctest --test-dir "$BUILD" --output-on-failure     | tee "$OUT/ctest-cuda.txt"

echo
echo "=== REAL CUDA OPERATION CAPABILITY CONTRACT ==="
STAGE="cuda-operation-contract"
"$BUILD/air-cuda-contract-tests"     2>&1 | tee "$OUT/cuda-contract.txt"

grep -Fq "CUDA prepared backend operation-site legality passed"     "$OUT/cuda-contract.txt"
grep -Fq "CUDA execution and dense tactic are independent of Qwen2 source names"     "$OUT/cuda-contract.txt"

wait_health() {
    for _ in $(seq 1 160); do
        if curl -fsS "http://127.0.0.1:$PORT/health" >/dev/null 2>&1; then
            return 0
        fi
        if [[ -s "$OUT/server.log" ]] &&
           grep -Eq 'failed|error|ERROR|fatal|FATAL' "$OUT/server.log"; then
            tail -n 100 "$OUT/server.log" >&2
        fi
        sleep 0.25
    done
    echo "server did not become healthy on port $PORT" >&2
    return 1
}

echo
echo "=== REAL MODEL CUDA NONREGRESSION ==="
STAGE="server-start"
"$BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$PORT"     --no-manifest     --execution-observation normal     > "$OUT/server.log" 2>&1 &
SERVER_PID=$!
wait_health

STAGE="server-generate"
curl -fsS --max-time 120     -H 'Content-Type: application/json'     -d '{"prompt":"AIR operation legality boundary.","max_tokens":4,"temperature":0.0}'     "http://127.0.0.1:$PORT/generate"     > "$OUT/generation.json"

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
assert int(metrics["request_id"]) > 0
assert int(metrics["sequence_id"]) > 0

planner = runtime["planner"]
assert planner["selected_backend"] == "cuda"

legal = {
    "prefill_block_quantized_linear": {
        "baseline", "batch-reuse4", "batch-reuse8", "dense-f32-cublas"
    },
    "decode_block_quantized_linear": {
        "baseline", "batch-reuse8", "dense-f32-cublas"
    },
    "decode_output_quantized_linear": {
        "baseline", "batch-reuse8"
    },
    "prefill_attention": {
        "baseline", "online-softmax"
    },
    "decode_attention": {
        "baseline"
    },
}
for key, allowed in legal.items():
    value = planner[key]
    assert value in allowed, f"{key} selected illegal implementation {value!r}"

print("prompt4_real_model_validation=PASS")
print("selected_backend=" + planner["selected_backend"])
for key in legal:
    print(f"{key}={planner[key]}")
PY

cleanup

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 4 qualification mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

if command -v nvidia-smi >/dev/null; then
    nvidia-smi         --query-gpu=index,name,uuid,memory.total,memory.free,driver_version,pstate,temperature.gpu,power.draw,clocks.sm,clocks.mem         --format=csv,noheader         > "$OUT/nvidia-smi.txt" || true
fi

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
names = [
    "identity.txt",
    "preflight-terminal.txt",
    "cmake-cuda.txt",
    "build-cuda.txt",
    "ctest-cuda.txt",
    "cuda-contract.txt",
    "server.log",
    "generation.json",
    "runtime.json",
    "nvidia-smi.txt",
]
files = [root / name for name in names if (root / name).is_file()]
with (root / "SHA256SUMS.txt").open("w") as out:
    for p in files:
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        out.write(f"{h}  {p.name}\n")
PY

trap - ERR
trap - EXIT
cleanup

echo
echo "PROMPT4_OPERATION_BOUNDARY=PASS"
echo "Evidence directory: $OUT"
