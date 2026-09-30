#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODEL_ROOT="${1:-$HOME/Models/Media/Image/FLUX.2-Klein-4B}"
COMFY_ROOT="${2:-$HOME/Projects/AI-Runtimes/ComfyUI}"
SELECTION_EVIDENCE="${3:-}"
PORT="${AIR_P7B_PORT:-8197}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P7B_OUT:-$HOME/Downloads/AIR-0.11-Prompt7B-FLUX2-Oracle-$STAMP}"

DIFFUSION="$MODEL_ROOT/flux-2-klein-4b-fp8.safetensors"
ENCODER="$MODEL_ROOT/split_files/text_encoders/qwen_3_4b_fp4_flux2.safetensors"
VAE="$MODEL_ROOT/split_files/vae/flux2-vae.safetensors"

VISIBLE_DIFFUSION="$COMFY_ROOT/models/diffusion_models/flux-2-klein-4b-fp8.safetensors"
VISIBLE_ENCODER="$COMFY_ROOT/models/text_encoders/qwen_3_4b_fp4_flux2.safetensors"
VISIBLE_VAE="$COMFY_ROOT/models/vae/flux2-vae.safetensors"

EXPECTED_AIR_BRANCH="architecture/adaptive-execution-substrate-r0"
EXPECTED_COMFY_HEAD="986c4d154ef8c288382ac87d956b52a2b640c8b3"
WORKFLOW_TEMPLATE_HEAD="0bfbbbfa260e76f69137f5aa37b7553199c73bc0"
BFL_REFERENCE_HEAD="50fe5162777813d869182b139e83b10743caef15"

mkdir -p "$OUT/output" "$OUT/temp" "$OUT/selection-evidence"
cd "$ROOT"

SERVER_PID=""
TELEMETRY_PID=""

cleanup() {
    set +e
    if [[ -n "$TELEMETRY_PID" ]]; then
        kill "$TELEMETRY_PID" 2>/dev/null || true
        wait "$TELEMETRY_PID" 2>/dev/null || true
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

echo "=== PROMPT 7B FLUX.2 KLEIN EXTERNAL ORACLE ==="
echo "output=$OUT"
echo

[[ "$(git branch --show-current)" == "$EXPECTED_AIR_BRANCH" ]] \
    || fail "run Prompt 7B from $EXPECTED_AIR_BRANCH"

[[ -z "$(git status --porcelain)" ]] \
    || fail "Prompt 7B requires a clean AIR worktree"

[[ -n "$SELECTION_EVIDENCE" ]] \
    || fail "third argument must be the qualified Prompt 7A selection evidence directory"

[[ -d "$SELECTION_EVIDENCE" ]] \
    || fail "selection evidence directory not found: $SELECTION_EVIDENCE"

grep -q '^selection_status=PROVISIONAL_QUALIFIED$' \
    "$SELECTION_EVIDENCE/selection.txt" \
    || fail "selection evidence is not provisionally qualified"

grep -q '97ed34fe0567e436200f2faee3939b88f2b5d99f8af2a4dc16532c4245c0ccb6' \
    "$SELECTION_EVIDENCE/artifact-sha256.txt" \
    || fail "selection evidence missing qualified diffusion identity"

grep -q '3eab03a77adb0ee5304a4e677d5c10ac22f9049c1d7c894adca4f8bb39206ca8' \
    "$SELECTION_EVIDENCE/artifact-sha256.txt" \
    || fail "selection evidence missing qualified text-encoder identity"

grep -q '868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3' \
    "$SELECTION_EVIDENCE/artifact-sha256.txt" \
    || fail "selection evidence missing qualified local VAE identity"

cp "$SELECTION_EVIDENCE/artifact-sha256.txt" "$OUT/selection-evidence/"
cp "$SELECTION_EVIDENCE/artifact-sizes.txt" "$OUT/selection-evidence/"
cp "$SELECTION_EVIDENCE/safetensors-census.json" "$OUT/selection-evidence/"
cp "$SELECTION_EVIDENCE/selection.txt" "$OUT/selection-evidence/"
cp "$SELECTION_EVIDENCE/comfy-git.txt" "$OUT/selection-evidence/"
cp "$SELECTION_EVIDENCE/comfy-import.txt" "$OUT/selection-evidence/"
cp "$SELECTION_EVIDENCE/comfy-model-visibility.json" "$OUT/selection-evidence/"

for path in "$DIFFUSION" "$ENCODER" "$VAE"; do
    [[ -f "$path" ]] || fail "selected artifact missing: $path"
done

echo "=== MODEL AUTHORITY / COMFYUI VISIBILITY ==="
for pair in \
    "$DIFFUSION|$VISIBLE_DIFFUSION" \
    "$ENCODER|$VISIBLE_ENCODER" \
    "$VAE|$VISIBLE_VAE"; do
    canonical="${pair%%|*}"
    visible="${pair#*|}"
    [[ -e "$visible" ]] || fail "ComfyUI-visible artifact missing: $visible"
    if [[ "$canonical" -ef "$visible" ]]; then
        echo "PASS same-file: $visible -> $(readlink -f "$visible")"
    else
        fail "ComfyUI-visible artifact is a separate file; canonical model authority would be ambiguous: $visible"
    fi
done | tee "$OUT/model-authority.txt"

COMFY_HEAD="$(git -C "$COMFY_ROOT" rev-parse HEAD)"
[[ "$COMFY_HEAD" == "$EXPECTED_COMFY_HEAD" ]] \
    || fail "ComfyUI head changed: expected $EXPECTED_COMFY_HEAD got $COMFY_HEAD"

[[ -z "$(git -C "$COMFY_ROOT" status --porcelain)" ]] \
    || fail "ComfyUI checkout must be clean for the external oracle"

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
        raise SystemExit(f"dedicated oracle port {port} unavailable: {exc}")
PY

echo
echo "=== GPU COMPUTE BASELINE ==="
COMPUTE_BEFORE="$(nvidia-smi --query-compute-apps=pid,process_name,used_memory --format=csv,noheader 2>/dev/null || true)"
printf '%s\n' "$COMPUTE_BEFORE" | tee "$OUT/compute-apps-before.txt"
[[ -z "$COMPUTE_BEFORE" ]] \
    || fail "GPU compute process already active; Prompt 7B requires a clean compute baseline"

{
    echo "date=$(date -Is)"
    echo "air_head=$(git rev-parse HEAD)"
    echo "air_branch=$(git branch --show-current)"
    echo "comfy_head=$COMFY_HEAD"
    echo "workflow_template_head=$WORKFLOW_TEMPLATE_HEAD"
    echo "bfl_reference_head=$BFL_REFERENCE_HEAD"
    echo "model_root=$MODEL_ROOT"
    echo "selection_evidence=$SELECTION_EVIDENCE"
    echo "port=$PORT"
    echo "oracle_runtime=ComfyUI-native"
    echo "custom_nodes=disabled"
    echo "cache=none"
    echo "deterministic=true"
    echo "resolution=1024x1024"
    echo "steps=4"
    echo "sampler=euler"
    echo "cfg=1"
    echo "seed=432262096973490"
} > "$OUT/identity.txt"

echo
echo "=== MACHINE BEFORE ==="
{
    free -h
    echo
    nvidia-smi
} | tee "$OUT/machine-before.txt"

echo
echo "=== START DEDICATED COMFYUI ORACLE ==="
(
    cd "$COMFY_ROOT"
    exec env PYTHONPATH="$COMFY_ROOT" "$PY" main.py \
        --listen 127.0.0.1 \
        --port "$PORT" \
        --disable-auto-launch \
        --disable-all-custom-nodes \
        --deterministic \
        --cache-none \
        --output-directory "$OUT/output" \
        --temp-directory "$OUT/temp" \
        --log-stdout
) > "$OUT/comfy-server.log" 2>&1 &
SERVER_PID=$!
echo "server_pid=$SERVER_PID" | tee -a "$OUT/identity.txt"

echo "unix_s,gpu_mem_used_mib,gpu_mem_free_mib,gpu_util_pct,temp_c,power_w,server_rss_kib,mem_available_kib" \
    > "$OUT/telemetry.csv"
(
    while kill -0 "$SERVER_PID" 2>/dev/null; do
        ts="$(date +%s.%N)"
        gpu="$(nvidia-smi \
            --query-gpu=memory.used,memory.free,utilization.gpu,temperature.gpu,power.draw \
            --format=csv,noheader,nounits 2>/dev/null | head -n1 | tr -d ' ')"
        rss="$(ps -o rss= -p "$SERVER_PID" 2>/dev/null | tr -d ' ' || true)"
        avail="$(awk '/MemAvailable:/ {print $2}' /proc/meminfo)"
        [[ -n "$rss" ]] || rss=0
        printf '%s,%s,%s,%s\n' "$ts" "$gpu" "$rss" "$avail"
        sleep 0.5
    done
) >> "$OUT/telemetry.csv" 2>/dev/null &
TELEMETRY_PID=$!

echo
echo "=== EXECUTE FIXED ORACLE TWICE ==="
set +e
"$PY" "$ROOT/scripts/prompt7b_flux2_oracle.py" \
    --base-url "http://127.0.0.1:$PORT" \
    --evidence-dir "$OUT" \
    --timeout 1200 \
    2>&1 | tee "$OUT/oracle-terminal.txt"
ORACLE_RC=${PIPESTATUS[0]}
set -e

if [[ "$ORACLE_RC" -ne 0 ]]; then
    fail "Prompt 7B oracle driver failed with rc=$ORACLE_RC"
fi

echo
echo "=== RESOURCE EVENTS ==="
grep -Ei \
    'load|loaded|loading|unload|offload|vram|ram|vae|clip|flux|model|prompt executed' \
    "$OUT/comfy-server.log" \
    > "$OUT/server-resource-events.txt" || true
tail -n 160 "$OUT/server-resource-events.txt" || true

echo
echo "=== TELEMETRY SUMMARY ==="
python3 - "$OUT/telemetry.csv" "$OUT/telemetry-summary.json" <<'PY'
import csv
import json
import pathlib
import statistics
import sys

src = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
rows = []
with src.open() as fh:
    for row in csv.DictReader(fh):
        try:
            rows.append({
                "unix_s": float(row["unix_s"]),
                "gpu_mem_used_mib": float(row["gpu_mem_used_mib"]),
                "gpu_mem_free_mib": float(row["gpu_mem_free_mib"]),
                "gpu_util_pct": float(row["gpu_util_pct"]),
                "temp_c": float(row["temp_c"]),
                "power_w": float(row["power_w"]),
                "server_rss_kib": int(row["server_rss_kib"]),
                "mem_available_kib": int(row["mem_available_kib"]),
            })
        except (ValueError, TypeError):
            continue

if not rows:
    raise SystemExit("no valid Prompt 7B telemetry samples")

summary = {
    "samples": len(rows),
    "peak_gpu_memory_used_mib": max(x["gpu_mem_used_mib"] for x in rows),
    "minimum_gpu_memory_free_mib": min(x["gpu_mem_free_mib"] for x in rows),
    "peak_gpu_utilization_percent": max(x["gpu_util_pct"] for x in rows),
    "peak_gpu_temperature_c": max(x["temp_c"] for x in rows),
    "peak_gpu_power_w": max(x["power_w"] for x in rows),
    "peak_server_rss_mib": max(x["server_rss_kib"] for x in rows) / 1024.0,
    "minimum_host_mem_available_gib": min(x["mem_available_kib"] for x in rows) / 1024.0 / 1024.0,
}
out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
for key, value in summary.items():
    print(f"{key}={value}")
PY

echo
echo "=== MACHINE AFTER ==="
{
    free -h
    echo
    nvidia-smi
} | tee "$OUT/machine-after.txt"

echo
echo "=== STOP DEDICATED ORACLE ==="
cleanup
SERVER_PID=""
TELEMETRY_PID=""

nvidia-smi \
    --query-compute-apps=pid,process_name,used_memory \
    --format=csv,noheader \
    > "$OUT/compute-apps-after.txt" 2>/dev/null || true

if [[ -s "$OUT/compute-apps-after.txt" ]]; then
    cat "$OUT/compute-apps-after.txt"
    fail "GPU compute process remained after dedicated oracle shutdown"
fi

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

[[ -z "$(git status --porcelain)" ]] \
    || fail "Prompt 7B mutated the AIR source worktree"

echo
echo "PROMPT7B_FLUX2_EXTERNAL_ORACLE=PASS"
echo "Evidence directory: $OUT"
