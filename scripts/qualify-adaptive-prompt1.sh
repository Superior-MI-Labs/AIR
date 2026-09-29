#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P1_OUT:-$HOME/Downloads/AIR-0.11-Prompt1-$STAMP}"

mkdir -p "$OUT"
cd "$ROOT"

STAGE="initialization"

on_error() {
    local rc=$?
    set +e
    echo >&2
    echo "PROMPT1_MACHINE_DISCOVERY=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2

    local log
    for log in         "$OUT/cmake-cpu.txt"         "$OUT/build-cpu.txt"         "$OUT/ctest-cpu.txt"         "$OUT/cmake-cuda.txt"         "$OUT/build-cuda.txt"         "$OUT/ctest-cuda.txt"; do
        if [[ -s "$log" ]]; then
            echo >&2
            echo "=== tail: $(basename "$log") ===" >&2
            tail -n 120 "$log" >&2
        fi
    done
    exit "$rc"
}
trap on_error ERR

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 1 qualification requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

HEAD="$(git rev-parse HEAD)"
BRANCH="$(git branch --show-current)"

{
    echo "date=$(date -Is)"
    echo "branch=$BRANCH"
    echo "head=$HEAD"
    echo "host=$(hostname)"
    echo "kernel=$(uname -srmo)"
} > "$OUT/identity.txt"

echo "=== CPU-ONLY BUILD ==="
STAGE="cpu-cmake-configure"
cmake -S "$ROOT" -B "$OUT/build-cpu"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=OFF     > "$OUT/cmake-cpu.txt" 2>&1

STAGE="cpu-build"
cmake --build "$OUT/build-cpu" -j"$(nproc)"     > "$OUT/build-cpu.txt" 2>&1

STAGE="cpu-ctest"
ctest --test-dir "$OUT/build-cpu" --output-on-failure     | tee "$OUT/ctest-cpu.txt"

"$OUT/build-cpu/air-cli" machine-info     | tee "$OUT/machine-info-cpu.txt"

STAGE="cpu-machine-json"
"$OUT/build-cpu/air-cli" machine-info --json     > "$OUT/machine-info-cpu.json"

STAGE="cpu-json-validation"
python3 - "$OUT/machine-info-cpu.json" <<'PY'
import json, pathlib, sys

p = pathlib.Path(sys.argv[1])
x = json.loads(p.read_text())
assert x["cuda_compiled"] is False
top = x["topology"]
env = x["environment"]
assert top["fingerprint"]
assert env["topology_fingerprint"] == top["fingerprint"]
assert env["observed_unix_ms"] > 0

kinds = {n["kind"] for n in top["nodes"]}
assert "cpu" in kinds
assert "host-memory" in kinds
assert "accelerator" not in kinds

ids = {n["id"] for n in top["nodes"]}
for r in env["resources"]:
    assert r["node_id"] in ids
print("cpu_machine_json=PASS")
PY

echo
echo "=== CUDA BUILD ==="

STAGE="cuda-preflight"
command -v nvcc >/dev/null || {
    echo "ERROR: nvcc is required for Prompt 1 CUDA qualification." >&2
    exit 3
}

STAGE="cuda-cmake-configure"
cmake -S "$ROOT" -B "$OUT/build-cuda"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=ON     > "$OUT/cmake-cuda.txt" 2>&1

STAGE="cuda-build"
cmake --build "$OUT/build-cuda" -j"$(nproc)"     > "$OUT/build-cuda.txt" 2>&1

STAGE="cuda-ctest"
ctest --test-dir "$OUT/build-cuda" --output-on-failure     | tee "$OUT/ctest-cuda.txt"

"$OUT/build-cuda/air-cli" machine-info     | tee "$OUT/machine-info-cuda.txt"

"$OUT/build-cuda/air-cli" machine-info --json     > "$OUT/machine-info-cuda-1.json"

sleep 1

"$OUT/build-cuda/air-cli" machine-info --json     > "$OUT/machine-info-cuda-2.json"

STAGE="cuda-json-validation"
python3 - "$OUT/machine-info-cuda-1.json" "$OUT/machine-info-cuda-2.json" <<'PY'
import json, pathlib, sys

a = json.loads(pathlib.Path(sys.argv[1]).read_text())
b = json.loads(pathlib.Path(sys.argv[2]).read_text())

assert a["cuda_compiled"] is True
assert b["cuda_compiled"] is True

ta, tb = a["topology"], b["topology"]
ea, eb = a["environment"], b["environment"]

assert ta["fingerprint"]
assert ta["fingerprint"] == tb["fingerprint"], (
    "stable topology fingerprint changed between repeated observations"
)
assert ea["topology_fingerprint"] == ta["fingerprint"]
assert eb["topology_fingerprint"] == tb["fingerprint"]
assert eb["observed_unix_ms"] >= ea["observed_unix_ms"]

nodes = ta["nodes"]
kinds = {n["kind"] for n in nodes}
assert "cpu" in kinds
assert "host-memory" in kinds
assert "accelerator" in kinds

gpus = [n for n in nodes if n["kind"] == "accelerator" and n["backend"] == "cuda"]
assert gpus, "CUDA build discovered no CUDA accelerator"
for gpu in gpus:
    assert gpu["total_bytes"] > 0
    assert str(gpu["architecture"]).startswith("sm")

node_by_id = {n["id"]: n for n in nodes}
for r in ea["resources"]:
    assert r["node_id"] in node_by_id
    total = int(node_by_id[r["node_id"]]["total_bytes"])
    if total:
        assert 0 <= int(r["available_bytes"]) <= total

print("cuda_machine_json=PASS")
print("topology_fingerprint=" + ta["fingerprint"])
print("cuda_devices=" + str(len(gpus)))
PY

if command -v nvidia-smi >/dev/null; then
    nvidia-smi         --query-gpu=index,name,uuid,memory.total,memory.free,driver_version,pstate,temperature.gpu,power.draw,clocks.sm,clocks.mem         --format=csv,noheader         > "$OUT/nvidia-smi.txt" || true
fi

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 1 qualification mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib, pathlib, sys

root = pathlib.Path(sys.argv[1])
files = [
    root / "identity.txt",
    root / "ctest-cpu.txt",
    root / "ctest-cuda.txt",
    root / "machine-info-cpu.json",
    root / "machine-info-cuda-1.json",
    root / "machine-info-cuda-2.json",
]
with (root / "SHA256SUMS.txt").open("w") as out:
    for p in files:
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        out.write(f"{h}  {p.name}\n")
PY

trap - ERR
echo
echo "PROMPT1_MACHINE_DISCOVERY=PASS"
echo "Evidence directory: $OUT"
