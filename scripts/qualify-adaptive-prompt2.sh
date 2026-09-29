#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18220}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P2_OUT:-$HOME/Downloads/AIR-0.11-Prompt2-$STAMP}"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [base-port]" >&2
    exit 2
fi

mkdir -p "$OUT"
cd "$ROOT"

CPU_PID=""
CUDA_PID=""
STAGE="initialization"

cleanup() {
    set +e
    if [[ -n "$CPU_PID" ]]; then
        kill -TERM "$CPU_PID" 2>/dev/null || true
        wait "$CPU_PID" 2>/dev/null || true
    fi
    if [[ -n "$CUDA_PID" ]]; then
        kill -TERM "$CUDA_PID" 2>/dev/null || true
        wait "$CUDA_PID" 2>/dev/null || true
    fi
}

on_error() {
    local rc=$?
    cleanup
    echo >&2
    echo "PROMPT2_TOPOLOGY_ENVIRONMENT=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in         "$OUT/prompt1-terminal.txt"         "$OUT/cpu-server.log"         "$OUT/cuda-server.log"; do
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
    echo "ERROR: Prompt 2 qualification requires a clean worktree." >&2
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
    for _ in $(seq 1 120); do
        if curl -fsS "http://127.0.0.1:$port/health" >/dev/null 2>&1; then
            return 0
        fi
        if [[ -s "$log" ]] && grep -Eq 'failed|error|ERROR|fatal|FATAL' "$log"; then
            tail -n 80 "$log" >&2
        fi
        sleep 0.25
    done
    echo "server did not become healthy on port $port" >&2
    return 1
}

echo "=== PROMPT 1 NONREGRESSION ==="
STAGE="prompt1-nonregression"
AIR_P1_OUT="$OUT/prompt1"     bash "$ROOT/scripts/qualify-adaptive-prompt1.sh"     2>&1 | tee "$OUT/prompt1-terminal.txt"

CPU_BUILD="$OUT/prompt1/build-cpu"
CUDA_BUILD="$OUT/prompt1/build-cuda"
CPU_PORT="$BASE_PORT"
CUDA_PORT="$((BASE_PORT + 1))"

echo
echo "=== CPU SERVER MACHINE AUTHORITY ==="
STAGE="cpu-server-start"
"$CPU_BUILD/air-server"     -m "$MODEL"     --backend reference     --host 127.0.0.1     --port "$CPU_PORT"     --no-manifest     > "$OUT/cpu-server.log" 2>&1 &
CPU_PID=$!
wait_health "$CPU_PORT" "$OUT/cpu-server.log"

STAGE="cpu-server-endpoints"
curl -fsS "http://127.0.0.1:$CPU_PORT/machine" > "$OUT/cpu-machine.json"
curl -fsS "http://127.0.0.1:$CPU_PORT/environment" > "$OUT/cpu-environment-1.json"
sleep 1
curl -fsS "http://127.0.0.1:$CPU_PORT/environment" > "$OUT/cpu-environment-2.json"

kill -TERM "$CPU_PID"
wait "$CPU_PID" || true
CPU_PID=""

echo
echo "=== CUDA SERVER MACHINE AUTHORITY ==="
STAGE="cuda-server-start"
"$CUDA_BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$CUDA_PORT"     --no-manifest     > "$OUT/cuda-server.log" 2>&1 &
CUDA_PID=$!
wait_health "$CUDA_PORT" "$OUT/cuda-server.log"

STAGE="cuda-server-endpoints"
curl -fsS "http://127.0.0.1:$CUDA_PORT/machine" > "$OUT/cuda-machine.json"
curl -fsS "http://127.0.0.1:$CUDA_PORT/environment" > "$OUT/cuda-environment-1.json"
sleep 1
curl -fsS "http://127.0.0.1:$CUDA_PORT/environment" > "$OUT/cuda-environment-2.json"

kill -TERM "$CUDA_PID"
wait "$CUDA_PID" || true
CUDA_PID=""

STAGE="endpoint-validation"
python3 - "$OUT" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

def load(name):
    return json.loads((root / name).read_text())

p1_cpu = load("prompt1/machine-info-cpu.json")
p1_cuda = load("prompt1/machine-info-cuda-1.json")

cpu_machine = load("cpu-machine.json")
cpu_env1 = load("cpu-environment-1.json")
cpu_env2 = load("cpu-environment-2.json")

cuda_machine = load("cuda-machine.json")
cuda_env1 = load("cuda-environment-1.json")
cuda_env2 = load("cuda-environment-2.json")

# Structural authority must match the pre-server CLI observation.
assert cpu_machine["fingerprint"] == p1_cpu["topology"]["fingerprint"]
assert cuda_machine["fingerprint"] == p1_cuda["topology"]["fingerprint"]

# Machine surface is structural only.
assert all("available_bytes" not in n for n in cpu_machine["nodes"])
assert all("available_bytes" not in n for n in cuda_machine["nodes"])

# Environment surface is volatile only and bound to structural identity.
for env, machine in [
    (cpu_env1, cpu_machine),
    (cpu_env2, cpu_machine),
    (cuda_env1, cuda_machine),
    (cuda_env2, cuda_machine),
]:
    assert env["topology_fingerprint"] == machine["fingerprint"]
    assert env["observed_unix_ms"] > 0
    assert isinstance(env["resources"], list)
    assert all("node_id" in r and "available_bytes" in r for r in env["resources"])

assert cpu_env2["observed_unix_ms"] >= cpu_env1["observed_unix_ms"]
assert cuda_env2["observed_unix_ms"] >= cuda_env1["observed_unix_ms"]

cpu_kinds = {n["kind"] for n in cpu_machine["nodes"]}
cuda_kinds = {n["kind"] for n in cuda_machine["nodes"]}
assert "cpu" in cpu_kinds and "host-memory" in cpu_kinds
assert "accelerator" not in cpu_kinds
assert "accelerator" in cuda_kinds

gpus = [n for n in cuda_machine["nodes"]
        if n["kind"] == "accelerator" and n["backend"] == "cuda"]
assert len(gpus) >= 1
assert any("RTX 3080 Laptop GPU" in n["name"] for n in gpus)
assert any(n["architecture"] == "sm86" for n in gpus)

cuda_resource_ids = {r["node_id"] for r in cuda_env1["resources"]}
assert any(n["id"] in cuda_resource_ids for n in gpus)

# The server has loaded the model, so environment availability can differ from
# Prompt 1 pre-server observation without changing machine identity. Record the
# delta instead of asserting a direction because unrelated processes may change.
pre = {r["node_id"]: int(r["available_bytes"])
       for r in p1_cuda["environment"]["resources"]}
during = {r["node_id"]: int(r["available_bytes"])
          for r in cuda_env1["resources"]}
for gpu in gpus:
    node_id = gpu["id"]
    if node_id in pre and node_id in during:
        print(f"{node_id}_available_pre_server={pre[node_id]}")
        print(f"{node_id}_available_with_model={during[node_id]}")
        print(f"{node_id}_availability_delta={during[node_id] - pre[node_id]}")

print("prompt2_endpoint_validation=PASS")
print("cpu_topology_fingerprint=" + cpu_machine["fingerprint"])
print("cuda_topology_fingerprint=" + cuda_machine["fingerprint"])
PY

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 2 qualification mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
names = [
    "identity.txt",
    "prompt1-terminal.txt",
    "cpu-machine.json",
    "cpu-environment-1.json",
    "cpu-environment-2.json",
    "cuda-machine.json",
    "cuda-environment-1.json",
    "cuda-environment-2.json",
    "cpu-server.log",
    "cuda-server.log",
]
with (root / "SHA256SUMS.txt").open("w") as out:
    for name in names:
        p = root / name
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        out.write(f"{h}  {name}\n")
PY

trap - ERR
trap - EXIT
cleanup

echo
echo "PROMPT2_TOPOLOGY_ENVIRONMENT=PASS"
echo "Evidence directory: $OUT"
