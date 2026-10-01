#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
PORT="${2:-18841}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P8C_OUT:-$HOME/Downloads/AIR-0.11-Prompt8C-WorkUnits-$STAMP}"
BUILD="$OUT/build-cuda"
BUILD_JOBS="${AIR_P8C_BUILD_JOBS:-4}"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [port]" >&2
    exit 2
fi
if [[ ! -f "$MODEL" ]]; then
    echo "ERROR: model not found: $MODEL" >&2
    exit 2
fi
if ! command -v nvidia-smi >/dev/null 2>&1; then
    echo "ERROR: nvidia-smi is required for Prompt 8C qualification" >&2
    exit 2
fi

mkdir -p "$OUT"
cd "$ROOT"

SERVER_PID=""
STAGE="initialization"

cleanup() {
    set +e
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
    echo "PROMPT8C_TYPED_WORK_UNITS=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in         "$OUT/preflight-terminal.txt"         "$OUT/cmake-cuda.txt"         "$OUT/build-cuda.txt"         "$OUT/ctest-cuda.txt"         "$OUT/core-graph-characterization.txt"         "$OUT/cuda-contract.txt"         "$OUT/detailed-server.log"         "$OUT/validation-terminal.txt"; do
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
    echo "ERROR: Prompt 8C requires a clean AIR worktree." >&2
    git status --short >&2
    exit 2
fi

if [[ "$(git branch --show-current)" != "architecture/adaptive-execution-substrate-r0" ]]; then
    echo "ERROR: Prompt 8C must run on architecture/adaptive-execution-substrate-r0" >&2
    exit 2
fi

STAGE="gpu-baseline"
compute_apps="$(
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader 2>/dev/null         | sed '/^[[:space:]]*$/d' || true
)"
printf '%s\n' "$compute_apps" > "$OUT/gpu-compute-baseline.txt"
if [[ -n "$compute_apps" ]]; then
    echo "ERROR: GPU compute baseline is not clean" >&2
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
    echo "build_jobs=$BUILD_JOBS"
} > "$OUT/identity.txt"

wait_health() {
    for _ in $(seq 1 160); do
        if curl -fsS "http://127.0.0.1:$PORT/health" >/dev/null 2>&1; then
            return 0
        fi
        if [[ -s "$OUT/detailed-server.log" ]] &&
           grep -Eq 'failed|error|ERROR|fatal|FATAL' "$OUT/detailed-server.log"; then
            tail -n 120 "$OUT/detailed-server.log" >&2
        fi
        sleep 0.25
    done
    echo "server did not become healthy on port $PORT" >&2
    return 1
}

echo "=== ADAPTIVE CPU PREFLIGHT ==="
STAGE="cpu-preflight"
AIR_BUILD_JOBS="$BUILD_JOBS" AIR_PREFLIGHT_OUT="$OUT/preflight-build"     bash "$ROOT/scripts/preflight-adaptive.sh"     2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== FRESH CUDA BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=ON     > "$OUT/cmake-cuda.txt" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" -j"$BUILD_JOBS"     > "$OUT/build-cuda.txt" 2>&1

echo
echo "=== FULL CUDA CTEST ==="
STAGE="cuda-ctest"
ctest --test-dir "$BUILD" --output-on-failure     | tee "$OUT/ctest-cuda.txt"

echo
echo "=== EXECUTION GRAPH CHARACTERIZATION ==="
STAGE="graph-characterization"
"$BUILD/air-core-tests"     2>&1 | tee "$OUT/core-graph-characterization.txt"
grep -Fq "ExecutionGraph R0 characterization passed"     "$OUT/core-graph-characterization.txt"

echo
echo "=== CUDA GRAPH / OBSERVATION CONTRACT ==="
STAGE="cuda-contract"
"$BUILD/air-cuda-contract-tests"     2>&1 | tee "$OUT/cuda-contract.txt"
grep -Fq "CUDA prepared backend operation-site legality passed"     "$OUT/cuda-contract.txt"
grep -Fq "CUDA ExecutionGraph planned/observed concordance characterization passed"     "$OUT/cuda-contract.txt"
grep -Fq "CUDA execution and dense tactic are independent of Qwen2 source names"     "$OUT/cuda-contract.txt"

echo
echo "=== DETAILED REAL QWEN OBSERVATION ==="
STAGE="server-start"
"$BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$PORT"     --no-manifest     --execution-observation detailed     --execution-span-capacity 16384     > "$OUT/detailed-server.log" 2>&1 &
SERVER_PID=$!
wait_health

STAGE="generation"
curl -fsS --max-time 120     -H 'Content-Type: application/json'     -d '{"prompt":"AIR Prompt 8C typed work-unit qualification.","max_tokens":4,"temperature":0.0}'     "http://127.0.0.1:$PORT/generate"     > "$OUT/generation.json"

curl -fsS "http://127.0.0.1:$PORT/machine"     > "$OUT/machine.json"
curl -fsS "http://127.0.0.1:$PORT/timeline"     > "$OUT/timeline.json"
curl -fsS "http://127.0.0.1:$PORT/execution-graphs"     > "$OUT/execution-graphs.json"

STAGE="typed-work-validation"
python3 - "$OUT" <<'PY' 2>&1 | tee "$OUT/validation-terminal.txt"
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

def load(name):
    return json.loads((root / name).read_text())

generation = load("generation.json")
machine = load("machine.json")
timeline = load("timeline.json")
graphs = load("execution-graphs.json")

metrics = generation["metrics"]
assert metrics["backend"] == "cuda"
assert int(metrics["generated_tokens"]) > 0

assert int(timeline["schema_version"]) == 2
assert timeline["level"] == "detailed"
assert int(timeline["dropped_spans"]) == 0

spans = timeline["spans"]
assert spans

nonzero = [s for s in spans if int(s["work_units"]) > 0]
assert nonzero
assert all(s.get("work_unit_kind") is not None for s in nonzero)

service_nonzero = [
    s for s in nonzero
    if s["scope"] == "service"
]
assert service_nonzero
assert all(s["work_unit_kind"] == "tokens" for s in service_nonzero)

backend_transfers = [
    s for s in spans
    if s["scope"] == "backend" and
       s["category"] == "transfer" and
       int(s["work_units"]) > 0
]
assert backend_transfers
assert all(s["work_unit_kind"] == "bytes" for s in backend_transfers)

backend_sync = [
    s for s in spans
    if s["scope"] == "backend" and
       s["category"] == "synchronization"
]
assert backend_sync
assert all(int(s["work_units"]) == 0 for s in backend_sync)
assert all(s.get("work_unit_kind") is None for s in backend_sync)

assert graphs["level"] == "detailed"
assert graphs["topology_status"] == "ready"
assert graphs["topology_fingerprint"] == machine["fingerprint"]
assert int(graphs["derivation_failures"]) == 0
assert int(graphs["dropped_graphs"]) == 0
assert graphs["observations"]

gpu0 = [
    n for n in machine["nodes"]
    if n["kind"] == "accelerator" and
       n["backend"] == "cuda" and
       int(n["ordinal"]) == 0
]
assert len(gpu0) == 1
gpu_id = gpu0[0]["id"]

saw_prefill = False
saw_decode = False
for observation in graphs["observations"]:
    graph = observation["graph"]
    assert int(graph["schema_version"]) == 2
    assert graph["backend"] == "cuda"
    assert graph["work_unit_kind"] == "tokens"
    assert graph["topology_fingerprint"] == machine["fingerprint"]
    assert graph["hardware_resource_id"] == gpu_id
    assert graph["identity"].startswith("execution-graph:r0:")
    assert observation["backend_success"] is True
    assert observation["evidence_status"] == "concordant"
    assert observation["evidence_truncated"] is False
    assert int(observation["unexpected_transfer_spans"]) == 0
    assert int(observation["unexpected_synchronization_spans"]) == 0
    assert int(observation["planned_transfer_regions"]) == int(
        observation["matched_transfer_regions"]
    )
    assert int(observation["planned_synchronization_regions"]) == int(
        observation["matched_synchronization_regions"]
    )

    kind = graph["invocation"]
    assert kind in {"prefill-single", "decode-single"}
    saw_prefill = saw_prefill or kind == "prefill-single"
    saw_decode = saw_decode or kind == "decode-single"

assert saw_prefill and saw_decode

summary = {
    "schema": "air.prompt8c.typed-work-units.v1",
    "timeline_schema_version": int(timeline["schema_version"]),
    "graph_schema_versions": sorted({
        int(o["graph"]["schema_version"]) for o in graphs["observations"]
    }),
    "service_nonzero_spans": len(service_nonzero),
    "service_work_unit_kinds": sorted({
        s["work_unit_kind"] for s in service_nonzero
    }),
    "backend_transfer_spans": len(backend_transfers),
    "backend_transfer_work_unit_kinds": sorted({
        s["work_unit_kind"] for s in backend_transfers
    }),
    "backend_sync_spans": len(backend_sync),
    "backend_sync_unitless_zero_work": all(
        int(s["work_units"]) == 0 and s.get("work_unit_kind") is None
        for s in backend_sync
    ),
    "graph_observations": len(graphs["observations"]),
    "graph_work_unit_kinds": sorted({
        o["graph"]["work_unit_kind"] for o in graphs["observations"]
    }),
    "topology_fingerprint": machine["fingerprint"],
    "hardware_resource_id": gpu_id,
    "generated_tokens": int(metrics["generated_tokens"]),
}
(root / "summary.json").write_text(
    json.dumps(summary, indent=2, sort_keys=True) + "\n"
)

print("prompt8c_service_token_units=PASS")
print("prompt8c_cuda_transfer_byte_units=PASS")
print("prompt8c_zero_work_sync_unitless=PASS")
print("prompt8c_execution_graph_token_units=PASS")
print("prompt8c_graph_concordance=PASS")
print("service_nonzero_spans=" + str(len(service_nonzero)))
print("backend_transfer_spans=" + str(len(backend_transfers)))
print("backend_sync_spans=" + str(len(backend_sync)))
print("graph_observations=" + str(len(graphs["observations"])))
print("topology_fingerprint=" + machine["fingerprint"])
print("hardware_resource_id=" + gpu_id)
PY

cleanup

STAGE="gpu-cleanup"
sleep 1
remaining="$(
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader 2>/dev/null         | sed '/^[[:space:]]*$/d' || true
)"
printf '%s\n' "$remaining" > "$OUT/gpu-compute-final.txt"
if [[ -n "$remaining" ]]; then
    echo "ERROR: Prompt 8C left a GPU compute process alive" >&2
    cat "$OUT/gpu-compute-final.txt" >&2
    exit 4
fi

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 8C mutated the source worktree." >&2
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
names = [
    "identity.txt",
    "gpu-compute-baseline.txt",
    "preflight-terminal.txt",
    "cmake-cuda.txt",
    "build-cuda.txt",
    "ctest-cuda.txt",
    "core-graph-characterization.txt",
    "cuda-contract.txt",
    "detailed-server.log",
    "generation.json",
    "machine.json",
    "timeline.json",
    "execution-graphs.json",
    "validation-terminal.txt",
    "summary.json",
    "gpu-compute-final.txt",
    "nvidia-smi.txt",
]
out = root / "SHA256SUMS.txt"
with out.open("w") as handle:
    for name in names:
        path = root / name
        if not path.is_file():
            continue
        handle.write(
            f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n"
        )
print(f"checksums={out}")
PY

trap - ERR
trap - EXIT
cleanup

echo
echo "PROMPT8C_TYPED_WORK_UNITS=PASS"
echo "Evidence directory: $OUT"
