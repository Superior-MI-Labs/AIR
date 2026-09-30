#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18540}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P5C_OUT:-$HOME/Downloads/AIR-0.11-Prompt5C-$STAMP}"
BUILD="$OUT/build-cuda"
DETAIL_PORT="$BASE_PORT"
NORMAL_PORT="$((BASE_PORT + 1))"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [base-port]" >&2
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
    echo "PROMPT5C_GRAPH_EVIDENCE=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in \
        "$OUT/preflight-terminal.txt" \
        "$OUT/cmake-cuda.txt" \
        "$OUT/build-cuda.txt" \
        "$OUT/ctest-cuda.txt" \
        "$OUT/core-graph-characterization.txt" \
        "$OUT/cuda-contract.txt" \
        "$OUT/detailed-server.log" \
        "$OUT/normal-server.log"; do
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
    echo "ERROR: Prompt 5C qualification requires a clean worktree." >&2
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

wait_health() {
    local port="$1"
    local log="$2"
    for _ in $(seq 1 160); do
        if curl -fsS "http://127.0.0.1:$port/health" >/dev/null 2>&1; then
            return 0
        fi
        if [[ -s "$log" ]] &&
           grep -Eq 'failed|error|ERROR|fatal|FATAL' "$log"; then
            tail -n 100 "$log" >&2
        fi
        sleep 0.25
    done
    echo "server did not become healthy on port $port" >&2
    return 1
}

echo "=== ADAPTIVE CPU PREFLIGHT ==="
STAGE="cpu-preflight"
AIR_PREFLIGHT_OUT="$OUT/preflight-build" \
    bash "$ROOT/scripts/preflight-adaptive.sh" \
    2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== CUDA BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAIR_ENABLE_CUDA=ON \
    > "$OUT/cmake-cuda.txt" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" -j"$(nproc)" \
    > "$OUT/build-cuda.txt" 2>&1

echo
echo "=== CUDA CTEST ==="
STAGE="cuda-ctest"
ctest --test-dir "$BUILD" --output-on-failure \
    | tee "$OUT/ctest-cuda.txt"

echo
echo "=== EXECUTION GRAPH R0 NONREGRESSION ==="
STAGE="graph-r0-characterization"
"$BUILD/air-core-tests" \
    2>&1 | tee "$OUT/core-graph-characterization.txt"
grep -Fq "ExecutionGraph R0 characterization passed" \
    "$OUT/core-graph-characterization.txt"

echo
echo "=== CUDA GRAPH/EVIDENCE CONTRACT ==="
STAGE="cuda-graph-contract"
"$BUILD/air-cuda-contract-tests" \
    2>&1 | tee "$OUT/cuda-contract.txt"
grep -Fq "CUDA prepared backend operation-site legality passed" \
    "$OUT/cuda-contract.txt"
grep -Fq "CUDA ExecutionGraph planned/observed concordance characterization passed" \
    "$OUT/cuda-contract.txt"
grep -Fq "CUDA execution and dense tactic are independent of Qwen2 source names" \
    "$OUT/cuda-contract.txt"

echo
echo "=== DETAILED REAL-MODEL GRAPH/EVIDENCE ==="
STAGE="detailed-server-start"
"$BUILD/air-server" \
    -m "$MODEL" \
    --backend cuda \
    --device 0 \
    --host 127.0.0.1 \
    --port "$DETAIL_PORT" \
    --no-manifest \
    --execution-observation detailed \
    --execution-span-capacity 16384 \
    > "$OUT/detailed-server.log" 2>&1 &
SERVER_PID=$!
wait_health "$DETAIL_PORT" "$OUT/detailed-server.log"

STAGE="detailed-real-generation"
curl -fsS --max-time 120 \
    -H 'Content-Type: application/json' \
    -d '{"prompt":"AIR planned and observed execution evidence.","max_tokens":4,"temperature":0.0}' \
    "http://127.0.0.1:$DETAIL_PORT/generate" \
    > "$OUT/detailed-generation.json"

curl -fsS "http://127.0.0.1:$DETAIL_PORT/machine" \
    > "$OUT/detailed-machine.json"
curl -fsS "http://127.0.0.1:$DETAIL_PORT/timeline" \
    > "$OUT/detailed-timeline.json"
curl -fsS "http://127.0.0.1:$DETAIL_PORT/execution-graphs" \
    > "$OUT/detailed-execution-graphs.json"

STAGE="detailed-evidence-validation"
python3 - "$OUT" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

def load(name):
    return json.loads((root / name).read_text())

generation = load("detailed-generation.json")
machine = load("detailed-machine.json")
timeline = load("detailed-timeline.json")
graphs = load("detailed-execution-graphs.json")

metrics = generation["metrics"]
assert metrics["backend"] == "cuda"
assert int(metrics["generated_tokens"]) > 0

assert timeline["level"] == "detailed"
assert timeline["dropped_spans"] == 0
backend_spans = [s for s in timeline["spans"] if s["scope"] == "backend"]
assert backend_spans
assert any(s["category"] == "transfer" for s in backend_spans)
assert any(s["category"] == "synchronization" for s in backend_spans)

assert graphs["level"] == "detailed"
assert graphs["topology_status"] == "ready"
assert graphs["topology_fingerprint"] == machine["fingerprint"]
assert graphs["derivation_failures"] == 0
assert graphs["dropped_graphs"] == 0
assert graphs["observations"]

gpu0 = [
    n for n in machine["nodes"]
    if n["kind"] == "accelerator" and
       n["backend"] == "cuda" and
       int(n["ordinal"]) == 0
]
assert len(gpu0) == 1
gpu_id = gpu0[0]["id"]

phase_for_payload = {
    "input-tokens": {"h2d-prefill-token-enqueue"},
    "full-logits": {"d2h-logits-enqueue"},
    "greedy-result": {"d2h-greedy-result-enqueue"},
    "target-tokens": {"h2d-target-token-enqueue"},
    "target-logprob-results": {
        "d2h-target-logprobs-enqueue",
        "d2h-target-logprob-flag-enqueue",
    },
}

saw_prefill = False
saw_decode = False
for observation in graphs["observations"]:
    graph = observation["graph"]
    assert graph["backend"] == "cuda"
    assert graph["topology_fingerprint"] == machine["fingerprint"]
    assert graph["hardware_resource_id"] == gpu_id
    assert graph["identity"].startswith("execution-graph:r0:")
    assert observation["backend_success"] is True
    assert observation["evidence_status"] == "concordant"
    assert observation["evidence_truncated"] is False
    assert observation["unexpected_transfer_spans"] == 0
    assert observation["unexpected_synchronization_spans"] == 0
    assert observation["planned_transfer_regions"] == observation["matched_transfer_regions"]
    assert (
        observation["planned_synchronization_regions"] ==
        observation["matched_synchronization_regions"]
    )

    kind = graph["invocation"]
    assert kind in {"prefill-single", "decode-single"}
    saw_prefill = saw_prefill or kind == "prefill-single"
    saw_decode = saw_decode or kind == "decode-single"

    pairs = {
        (int(p["request_id"]), int(p["sequence_id"]))
        for p in observation["participants"]
    }
    assert pairs

    start = int(observation["start_ns"])
    end = int(observation["end_ns"])
    evidence = [
        s for s in backend_spans
        if (int(s["request_id"]), int(s["sequence_id"])) in pairs and
           int(s["start_ns"]) >= start and
           int(s["end_ns"]) <= end
    ]

    for node in graph["nodes"]:
        if node["kind"] == "transfer-region":
            payload = node["payload"]
            expected = phase_for_payload[payload]
            assert any(
                s["category"] == "transfer" and s["phase"] in expected
                for s in evidence
            ), (payload, evidence)
        elif node["kind"] == "synchronization-region":
            assert any(
                s["category"] == "synchronization" and
                s["phase"].startswith("cuda-stream-wait-")
                for s in evidence
            ), evidence

assert saw_prefill and saw_decode
print("prompt5c_detailed_graph_evidence=PASS")
print("graph_observations=" + str(len(graphs["observations"])))
print("backend_spans=" + str(len(backend_spans)))
print("topology_fingerprint=" + machine["fingerprint"])
print("hardware_resource_id=" + gpu_id)
PY

cleanup

echo
echo "=== NORMAL-MODE NONINTRUSION ==="
STAGE="normal-server-start"
"$BUILD/air-server" \
    -m "$MODEL" \
    --backend cuda \
    --device 0 \
    --host 127.0.0.1 \
    --port "$NORMAL_PORT" \
    --no-manifest \
    --execution-observation normal \
    > "$OUT/normal-server.log" 2>&1 &
SERVER_PID=$!
wait_health "$NORMAL_PORT" "$OUT/normal-server.log"

STAGE="normal-generation"
curl -fsS --max-time 120 \
    -H 'Content-Type: application/json' \
    -d '{"prompt":"AIR normal observation remains graph-free.","max_tokens":4,"temperature":0.0}' \
    "http://127.0.0.1:$NORMAL_PORT/generate" \
    > "$OUT/normal-generation.json"
curl -fsS "http://127.0.0.1:$NORMAL_PORT/execution-graphs" \
    > "$OUT/normal-execution-graphs.json"

STAGE="normal-nonintrusion-validation"
python3 - "$OUT/normal-execution-graphs.json" <<'PY'
import json
import pathlib
import sys

graphs = json.loads(pathlib.Path(sys.argv[1]).read_text())
assert graphs["level"] == "normal"
assert graphs["topology_status"] == "disabled"
assert graphs["derivation_failures"] == 0
assert graphs["dropped_graphs"] == 0
assert graphs["observations"] == []
print("prompt5c_normal_graph_nonintrusion=PASS")
PY

cleanup

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 5C qualification mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

if command -v nvidia-smi >/dev/null; then
    nvidia-smi \
        --query-gpu=index,name,uuid,memory.total,memory.free,driver_version,pstate,temperature.gpu,power.draw,clocks.sm,clocks.mem \
        --format=csv,noheader \
        > "$OUT/nvidia-smi.txt" || true
fi

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
files = [
    p for p in root.rglob("*")
    if p.is_file() and p.name != "SHA256SUMS.txt"
]
with (root / "SHA256SUMS.txt").open("w") as out:
    for p in sorted(files):
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        out.write(f"{h}  {p.relative_to(root)}\n")
PY

trap - ERR
trap - EXIT
cleanup

echo
echo "PROMPT5C_GRAPH_EVIDENCE=PASS"
echo "Evidence directory: $OUT"
