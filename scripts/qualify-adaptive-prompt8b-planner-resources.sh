#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
P6C="${2:-$HOME/Downloads/AIR-0.11-Prompt6C-Transitions-20260930-100245}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P8B_PLANNER_OUT:-$HOME/Downloads/AIR-0.11-Prompt8B-Planner-Resources-$STAMP}"
BUILD="$OUT/build-cuda"
BUILD_JOBS="${AIR_P8B_BUILD_JOBS:-4}"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [prompt6c-evidence-dir]" >&2
    exit 2
fi

MANIFEST="$P6C/final-manifest.json"
MEDIUM="$P6C/prompts/medium.txt"
SMALL="$P6C/prompts/small.txt"

for path in "$MODEL" "$MANIFEST" "$MEDIUM" "$SMALL"; do
    if [[ ! -f "$path" ]]; then
        echo "ERROR: required input not found: $path" >&2
        exit 2
    fi
done

if ! command -v nvidia-smi >/dev/null 2>&1; then
    echo "ERROR: nvidia-smi is required" >&2
    exit 2
fi

mkdir -p "$OUT"
cd "$ROOT"

STAGE="initialization"

on_error() {
    local rc=$?
    echo >&2
    echo "PROMPT8B_PLANNER_RESOURCE_AUTHORITY=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in         "$OUT/preflight-terminal.txt"         "$OUT/cmake-cuda.txt"         "$OUT/build-cuda.txt"         "$OUT/ctest-cuda.txt"         "$OUT/hot-dense.log"         "$OUT/eviction.log"         "$OUT/validation-terminal.txt"; do
        if [[ -s "$log" ]]; then
            echo >&2
            echo "=== tail: $(basename "$log") ===" >&2
            tail -n 180 "$log" >&2
        fi
    done
    exit "$rc"
}
trap on_error ERR

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 8B planner qualification requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

if [[ "$(git branch --show-current)" != "architecture/adaptive-execution-substrate-r0" ]]; then
    echo "ERROR: wrong branch" >&2
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
    echo "prompt6c_evidence=$P6C"
    echo "manifest_sha256=$(sha256sum "$MANIFEST" | awk '{print $1}')"
    echo "medium_prompt_sha256=$(sha256sum "$MEDIUM" | awk '{print $1}')"
    echo "small_prompt_sha256=$(sha256sum "$SMALL" | awk '{print $1}')"
    echo "build_jobs=$BUILD_JOBS"
} > "$OUT/identity.txt"

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
echo "=== HOT DENSE IDENTITY REPLAY ==="
STAGE="hot-dense"
"$BUILD/air-strategy-probe"     -m "$MODEL"     --manifest "$MANIFEST"     --medium-prompt-file "$MEDIUM"     --small-prompt-file "$SMALL"     --scenario hot-dense     --horizon-tokens 0     --output "$OUT/hot-dense.json"     > "$OUT/hot-dense.log" 2>&1

echo
echo "=== DENSE TO REUSE8 EVICTION REPLAY ==="
STAGE="eviction"
"$BUILD/air-strategy-probe"     -m "$MODEL"     --manifest "$MANIFEST"     --medium-prompt-file "$MEDIUM"     --small-prompt-file "$SMALL"     --scenario eviction     --horizon-tokens 0     --output "$OUT/eviction.json"     > "$OUT/eviction.log" 2>&1

echo
echo "=== IDENTITY-AWARE PLANNER VALIDATION ==="
STAGE="validation"
python3 - "$OUT/hot-dense.json" "$OUT/eviction.json" "$OUT/summary.json" <<'PY'     2>&1 | tee "$OUT/validation-terminal.txt"
import json
import pathlib
import sys

hot_path = pathlib.Path(sys.argv[1])
eviction_path = pathlib.Path(sys.argv[2])
summary_path = pathlib.Path(sys.argv[3])

hot = json.loads(hot_path.read_text())
eviction = json.loads(eviction_path.read_text())

DENSE_ID = "cuda/linear/dense-f32-cublas"

def resource(snapshot, resource_id):
    resources = snapshot.get("current_prepared_resources")
    assert isinstance(resources, list), "snapshot missing current_prepared_resources"
    matches = [x for x in resources if x.get("resource_id") == resource_id]
    assert len(matches) == 1, f"expected exactly one {resource_id} record: {matches}"
    return matches[0]

hot_steps = hot["steps"]
assert len(hot_steps) == 2
assert [x["metrics"]["strategy_id"] for x in hot_steps] == [
    "dense-medium", "dense-medium"
]

second = hot_steps[1]
assert bool(second["metrics"]["strategy_prepared_state_hot"]) is True
assert int(second["metrics"]["plan_preparation_bytes"]) == 0

hot_before = resource(second["before"], DENSE_ID)
hot_after = resource(second["after"], DENSE_ID)
assert hot_before["state"] == "resident"
assert int(hot_before["device_bytes"]) > 0
assert hot_after["state"] == "resident"
assert int(hot_after["device_bytes"]) == int(hot_before["device_bytes"])
assert int(second["before"]["current_prepared_artifact_bytes"]) == int(
    hot_before["device_bytes"]
)
assert int(second["after"]["current_prepared_artifact_bytes"]) == int(
    hot_after["device_bytes"]
)

ev_steps = eviction["steps"]
assert len(ev_steps) == 2
assert [x["metrics"]["strategy_id"] for x in ev_steps] == [
    "dense-medium", "reuse8-small"
]

evict = ev_steps[1]
assert float(evict["metrics"]["plan_eviction_ms"]) > 0.0

evict_before = resource(evict["before"], DENSE_ID)
evict_after = resource(evict["after"], DENSE_ID)
assert evict_before["state"] == "resident"
assert int(evict_before["device_bytes"]) > 0
assert evict_after["state"] == "nonresident"
assert int(evict_after["device_bytes"]) == 0
assert int(evict["after"]["current_prepared_artifact_bytes"]) == 0

final_dense = resource(eviction["snapshot"], DENSE_ID)
assert final_dense["state"] == "nonresident"
assert int(final_dense["device_bytes"]) == 0

summary = {
    "schema": "air.prompt8b.planner-resource-authority.v1",
    "dense_resource_id": DENSE_ID,
    "hot_second_strategy": second["metrics"]["strategy_id"],
    "hot_second_prepared_state_hot": bool(
        second["metrics"]["strategy_prepared_state_hot"]
    ),
    "hot_second_preparation_bytes": int(
        second["metrics"]["plan_preparation_bytes"]
    ),
    "hot_dense_resident_bytes": int(hot_before["device_bytes"]),
    "eviction_second_strategy": evict["metrics"]["strategy_id"],
    "eviction_ms": float(evict["metrics"]["plan_eviction_ms"]),
    "dense_state_after_eviction": evict_after["state"],
    "dense_bytes_after_eviction": int(evict_after["device_bytes"]),
    "hidden_dense_residency_after_eviction": False,
}
summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")

print("prompt8b_hot_identity=PASS")
print("prompt8b_eviction_identity=PASS")
print("dense_resource_id=" + DENSE_ID)
print("hot_dense_resident_bytes=" + str(summary["hot_dense_resident_bytes"]))
print("eviction_ms=" + str(summary["eviction_ms"]))
print("hidden_dense_residency_after_eviction=NO")
PY

STAGE="gpu-cleanup"
sleep 1
remaining="$(
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader 2>/dev/null         | sed '/^[[:space:]]*$/d' || true
)"
printf '%s\n' "$remaining" > "$OUT/gpu-compute-final.txt"
if [[ -n "$remaining" ]]; then
    echo "ERROR: Prompt 8B planner replay left a GPU compute process alive" >&2
    cat "$OUT/gpu-compute-final.txt" >&2
    exit 4
fi

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: qualification mutated source worktree" >&2
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
    "hot-dense.log",
    "hot-dense.json",
    "eviction.log",
    "eviction.json",
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
        handle.write(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n")
print(f"checksums={out}")
PY

trap - ERR

echo
echo "PROMPT8B_PLANNER_RESOURCE_AUTHORITY=PASS"
echo "Evidence directory: $OUT"
