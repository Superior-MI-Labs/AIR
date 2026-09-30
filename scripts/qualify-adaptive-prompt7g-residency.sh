#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ORACLE_DIR="${1:-$HOME/Downloads/AIR-0.11-Prompt7B-FLUX2-Oracle-20260930-145158}"
CENSUS_DIR="${2:-}"
COMFY_ROOT="${3:-$HOME/Projects/AI-Runtimes/ComfyUI}"
PORT="${AIR_P7G_PORT:-8198}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P7G_OUT:-$HOME/Downloads/AIR-0.11-Prompt7G-FLUX2-Residency-$STAMP}"

EXPECTED_AIR_BRANCH="architecture/adaptive-execution-substrate-r0"
EXPECTED_COMFY_HEAD="986c4d154ef8c288382ac87d956b52a2b640c8b3"
EXPECTED_PIXEL_SHA256="c3a4278c608408df5019cf15162707e29263dee76e7a0114a6b1dcf2c29e1aa6"

mkdir -p "$OUT/output" "$OUT/temp"
cd "$ROOT"

SERVER_PID=""
GPU_TELEMETRY_PID=""

cleanup() {
    if [[ -n "$GPU_TELEMETRY_PID" ]]; then
        kill "$GPU_TELEMETRY_PID" 2>/dev/null || true
        wait "$GPU_TELEMETRY_PID" 2>/dev/null || true
    fi
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        for _ in $(seq 1 20); do
            kill -0 "$SERVER_PID" 2>/dev/null || break
            sleep 0.25
        done
        kill -KILL "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

fail() {
    echo "ERROR: $*" >&2
    if [[ -f "$OUT/comfy-server.log" ]]; then
        echo >&2
        echo "=== tail: comfy-server.log ===" >&2
        tail -n 160 "$OUT/comfy-server.log" >&2 || true
    fi
    exit 1
}

echo "=== PROMPT 7G FLUX.2 RESIDENCY / TRANSITION OBSERVABILITY ==="
echo "oracle=$ORACLE_DIR"
echo "census=$CENSUS_DIR"
echo "comfy=$COMFY_ROOT"
echo "output=$OUT"
echo

[[ "$(git branch --show-current)" == "$EXPECTED_AIR_BRANCH" ]]     || fail "run Prompt 7G from $EXPECTED_AIR_BRANCH"

[[ -z "$(git status --porcelain)" ]]     || fail "Prompt 7G requires a clean AIR worktree"

[[ -n "$CENSUS_DIR" && -d "$CENSUS_DIR" ]]     || fail "second argument must be the qualified Prompt 7C-H evidence directory"

[[ -f "$ORACLE_DIR/oracle-summary.json" ]]     || fail "qualified Prompt 7B oracle summary missing"

[[ -f "$CENSUS_DIR/prompt7c-h-summary.json" ]]     || fail "Prompt 7C-H summary missing"

[[ -f "$CENSUS_DIR/unresolved-evidence.json" ]]     || fail "Prompt 7C-H unresolved evidence record missing"

python3 - "$ORACLE_DIR/oracle-summary.json" "$CENSUS_DIR/prompt7c-h-summary.json" "$CENSUS_DIR/unresolved-evidence.json" "$EXPECTED_PIXEL_SHA256" <<'PY'
import json
import pathlib
import sys

oracle = json.loads(pathlib.Path(sys.argv[1]).read_text())
census = json.loads(pathlib.Path(sys.argv[2]).read_text())
unresolved = json.loads(pathlib.Path(sys.argv[3]).read_text())
expected = sys.argv[4]

hashes = {x.get("pixel_sha256") for x in oracle.get("runs", [])}
if hashes != {expected} or oracle.get("same_pixel_sha256") is not True:
    raise SystemExit("Prompt 7B oracle identity/reproducibility changed")
if census.get("oracle_pixel_sha256") != expected:
    raise SystemExit("Prompt 7C-H census is not bound to the qualified oracle")
ids = {x.get("id") for x in unresolved.get("items", [])}
required = {"component-transition-timing", "component-residency-timeline", "conditioning-runtime-shape"}
if ids != required:
    raise SystemExit(f"Prompt 7C-H unresolved set changed: {sorted(ids)}")
print("retained_prompt7_evidence=PASS")
PY

COMFY_HEAD="$(git -C "$COMFY_ROOT" rev-parse HEAD)"
[[ "$COMFY_HEAD" == "$EXPECTED_COMFY_HEAD" ]]     || fail "ComfyUI head changed: expected $EXPECTED_COMFY_HEAD got $COMFY_HEAD"

[[ -z "$(git -C "$COMFY_ROOT" status --porcelain)" ]]     || fail "ComfyUI checkout must remain clean for Prompt 7G"

PY="$COMFY_ROOT/.venv/bin/python"
[[ -x "$PY" ]] || fail "ComfyUI venv python missing: $PY"

python3 - "$PORT" <<'PY'
import socket
import sys
port = int(sys.argv[1])
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:
        s.bind(("127.0.0.1", port))
    except OSError as exc:
        raise SystemExit(f"dedicated Prompt 7G port {port} unavailable: {exc}")
PY

echo
echo "=== GPU COMPUTE BASELINE ==="
COMPUTE_BEFORE="$(nvidia-smi --query-compute-apps=pid,process_name,used_memory --format=csv,noheader 2>/dev/null || true)"
printf '%s\n' "$COMPUTE_BEFORE" | tee "$OUT/compute-apps-before.txt"
[[ -z "$COMPUTE_BEFORE" ]]     || fail "GPU compute process already active; Prompt 7G requires a clean compute baseline"

{
    echo "date=$(date -Is)"
    echo "air_head=$(git rev-parse HEAD)"
    echo "air_branch=$(git branch --show-current)"
    echo "comfy_head=$COMFY_HEAD"
    echo "oracle_dir=$ORACLE_DIR"
    echo "census_dir=$CENSUS_DIR"
    echo "port=$PORT"
    echo "probe=source-clean launch-time model-management observer"
    echo "measurement_host_call_duration=true"
    echo "measurement_runtime_loaded_bytes=true"
    echo "measurement_external_gpu_telemetry=100ms"
    echo "claim_async_transfer_completion=false"
    echo "claim_per_layer_dynamic_residency=false"
} > "$OUT/identity.txt"

echo
echo "=== START PINNED COMFYUI THROUGH SOURCE-CLEAN PROBE ==="
(
    cd "$COMFY_ROOT"
    exec env PYTHONPATH="$COMFY_ROOT" "$PY"         "$ROOT/scripts/prompt7g_residency_probe.py" serve         --comfy-root "$COMFY_ROOT"         --event-log "$OUT/transition-events.jsonl"         --         --listen 127.0.0.1         --port "$PORT"         --disable-auto-launch         --disable-all-custom-nodes         --deterministic         --cache-none         --output-directory "$OUT/output"         --temp-directory "$OUT/temp"         --log-stdout
) > "$OUT/comfy-server.log" 2>&1 &
SERVER_PID=$!
echo "server_pid=$SERVER_PID" >> "$OUT/identity.txt"

echo
echo "=== START 100MS DEVICE TELEMETRY ==="
(
    exec stdbuf -oL -eL nvidia-smi         --query-gpu=timestamp,memory.used,memory.free,utilization.gpu,temperature.gpu,power.draw         --format=csv,noheader,nounits         --loop-ms=100
) > "$OUT/gpu-telemetry.csv" 2>/dev/null &
GPU_TELEMETRY_PID=$!

echo
echo "=== EXECUTE UNCHANGED QUALIFIED ORACLE ==="
set +e
"$PY" "$ROOT/scripts/prompt7b_flux2_oracle.py"     --base-url "http://127.0.0.1:$PORT"     --evidence-dir "$OUT"     --timeout 1200     2>&1 | tee "$OUT/oracle-terminal.txt"
ORACLE_RC=${PIPESTATUS[0]}
set -e

[[ "$ORACLE_RC" -eq 0 ]]     || fail "Prompt 7G unchanged oracle failed with rc=$ORACLE_RC"

echo
echo "=== FORCE POST-ORACLE UNLOAD FOR EVICTION OBSERVATION ==="
"$PY" - "$PORT" <<'PY'
import json
import sys
import time
import urllib.request

port = int(sys.argv[1])
body = json.dumps({"unload_models": True, "free_memory": True}).encode("utf-8")
req = urllib.request.Request(
    f"http://127.0.0.1:{port}/free",
    data=body,
    headers={"Content-Type": "application/json"},
)
with urllib.request.urlopen(req, timeout=30) as response:
    if response.status != 200:
        raise SystemExit(f"/free returned HTTP {response.status}")
time.sleep(5.0)
print("post_oracle_free=REQUESTED")
PY

echo
echo "=== ANALYZE TRANSITION / RESIDENCY EVIDENCE ==="
"$PY" "$ROOT/scripts/prompt7g_residency_probe.py" analyze     --event-log "$OUT/transition-events.jsonl"     --probe-oracle-summary "$OUT/oracle-summary.json"     --baseline-oracle-summary "$ORACLE_DIR/oracle-summary.json"     --census-summary "$CENSUS_DIR/prompt7c-h-summary.json"     --gpu-telemetry "$OUT/gpu-telemetry.csv"     --output "$OUT/prompt7g-summary.json"     2>&1 | tee "$OUT/analysis-terminal.txt"

echo
echo "=== RESOURCE EVENT EXCERPT ==="
grep -Ei     'load|loaded|loading|unload|offload|vram|ram|vae|clip|flux|model|prompt executed'     "$OUT/comfy-server.log"     > "$OUT/server-resource-events.txt" || true
tail -n 160 "$OUT/server-resource-events.txt" || true

echo
echo "=== STOP DEDICATED PROBE ==="
cleanup
SERVER_PID=""
GPU_TELEMETRY_PID=""

nvidia-smi     --query-compute-apps=pid,process_name,used_memory     --format=csv,noheader     > "$OUT/compute-apps-after.txt" 2>/dev/null || true

if [[ -s "$OUT/compute-apps-after.txt" ]]; then
    cat "$OUT/compute-apps-after.txt"
    fail "GPU compute process remained after Prompt 7G shutdown"
fi

[[ -z "$(git status --porcelain)" ]]     || fail "Prompt 7G mutated the AIR source worktree"

[[ -z "$(git -C "$COMFY_ROOT" status --porcelain)" ]]     || fail "Prompt 7G mutated the pinned ComfyUI checkout"

echo
echo "=== EVIDENCE CHECKSUMS ==="
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
out = root / "SHA256SUMS.txt"
with out.open("w") as fh:
    for path in sorted(p for p in root.rglob("*") if p.is_file() and p.name != out.name):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        fh.write(f"{digest}  {path.relative_to(root)}\n")
PY

echo
echo "PROMPT7G_FLUX2_RESIDENCY_TRANSITION=PASS"
echo "Evidence directory: $OUT"
