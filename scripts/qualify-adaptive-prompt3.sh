#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18320}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P3_OUT:-$HOME/Downloads/AIR-0.11-Prompt3-$STAMP}"

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
    echo "PROMPT3_EXECUTION_OBSERVATION=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in         "$OUT/preflight-terminal.txt"         "$OUT/prompt2-terminal.txt"         "$OUT/reference-normal.log"         "$OUT/cuda-off.log"         "$OUT/cuda-normal.log"         "$OUT/cuda-detailed.log"; do
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
    echo "ERROR: Prompt 3 qualification requires a clean worktree." >&2
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
        if [[ -s "$log" ]] && grep -Eq 'failed|error|ERROR|fatal|FATAL' "$log"; then
            tail -n 100 "$log" >&2
        fi
        sleep 0.25
    done
    echo "server did not become healthy on port $port" >&2
    return 1
}

stop_server() {
    set +e
    if [[ -n "$SERVER_PID" ]]; then
        kill -TERM "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
        SERVER_PID=""
    fi
    set -e
}

request_json() {
    local port="$1"
    local max_tokens="$2"
    curl -fsS --max-time 120 \
        -H 'Content-Type: application/json' \
        -d "{\"prompt\":\"AIR observes execution evidence.\",\"max_tokens\":$max_tokens,\"temperature\":0.0}" \
        "http://127.0.0.1:$port/generate"
}

echo "=== ADAPTIVE PREFLIGHT ==="
STAGE="adaptive-preflight"
AIR_PREFLIGHT_OUT="$OUT/preflight-build"     bash "$ROOT/scripts/preflight-adaptive.sh"     2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== PROMPT 2 NONREGRESSION ==="
STAGE="prompt2-nonregression"
AIR_P2_OUT="$OUT/prompt2"     bash "$ROOT/scripts/qualify-adaptive-prompt2.sh" "$MODEL" "$BASE_PORT"     2>&1 | tee "$OUT/prompt2-terminal.txt"

CPU_BUILD="$OUT/prompt2/prompt1/build-cpu"
CUDA_BUILD="$OUT/prompt2/prompt1/build-cuda"

REF_PORT="$((BASE_PORT + 10))"
CUDA_OFF_PORT="$((BASE_PORT + 11))"
CUDA_NORMAL_PORT="$((BASE_PORT + 12))"
CUDA_DETAILED_PORT="$((BASE_PORT + 13))"

echo
echo "=== REFERENCE NORMAL TIMELINE ==="
STAGE="reference-normal-start"
"$CPU_BUILD/air-server"     -m "$MODEL"     --backend reference     --host 127.0.0.1     --port "$REF_PORT"     --no-manifest     --execution-observation normal     --execution-span-capacity 2048     > "$OUT/reference-normal.log" 2>&1 &
SERVER_PID=$!
wait_health "$REF_PORT" "$OUT/reference-normal.log"

STAGE="reference-normal-request"
request_json "$REF_PORT" 2 > "$OUT/reference-normal-response.json"
curl -fsS "http://127.0.0.1:$REF_PORT/timeline"     > "$OUT/reference-normal-timeline.json"
stop_server

echo
echo "=== CUDA OFF TIMELINE ==="
STAGE="cuda-off-start"
"$CUDA_BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$CUDA_OFF_PORT"     --no-manifest     --execution-observation off     --execution-span-capacity 2048     > "$OUT/cuda-off.log" 2>&1 &
SERVER_PID=$!
wait_health "$CUDA_OFF_PORT" "$OUT/cuda-off.log"

STAGE="cuda-off-request"
request_json "$CUDA_OFF_PORT" 24 > "$OUT/cuda-off-response.json"
curl -fsS "http://127.0.0.1:$CUDA_OFF_PORT/timeline"     > "$OUT/cuda-off-timeline.json"
stop_server

echo
echo "=== CUDA NORMAL TIMELINE ==="
STAGE="cuda-normal-start"
"$CUDA_BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$CUDA_NORMAL_PORT"     --no-manifest     --execution-observation normal     --execution-span-capacity 4096     > "$OUT/cuda-normal.log" 2>&1 &
SERVER_PID=$!
wait_health "$CUDA_NORMAL_PORT" "$OUT/cuda-normal.log"

STAGE="cuda-normal-request"
request_json "$CUDA_NORMAL_PORT" 24 > "$OUT/cuda-normal-response.json"
curl -fsS "http://127.0.0.1:$CUDA_NORMAL_PORT/timeline"     > "$OUT/cuda-normal-timeline.json"
stop_server

echo
echo "=== CUDA DETAILED TIMELINE ==="
STAGE="cuda-detailed-start"
"$CUDA_BUILD/air-server"     -m "$MODEL"     --backend cuda     --device 0     --host 127.0.0.1     --port "$CUDA_DETAILED_PORT"     --no-manifest     --execution-observation detailed     --execution-span-capacity 8192     > "$OUT/cuda-detailed.log" 2>&1 &
SERVER_PID=$!
wait_health "$CUDA_DETAILED_PORT" "$OUT/cuda-detailed.log"

STAGE="cuda-detailed-request"
request_json "$CUDA_DETAILED_PORT" 24 > "$OUT/cuda-detailed-response.json"
curl -fsS "http://127.0.0.1:$CUDA_DETAILED_PORT/timeline"     > "$OUT/cuda-detailed-timeline.json"
stop_server

echo
echo "=== OBSERVER OVERHEAD SAMPLES ==="
STAGE="cuda-detailed-overhead"
python3 - "$CUDA_BUILD/air-server" "$MODEL" "$OUT" "$((BASE_PORT + 20))" <<'PY'
import json
import pathlib
import statistics
import subprocess
import sys
import time
import urllib.request

server = sys.argv[1]
model = sys.argv[2]
out_dir = pathlib.Path(sys.argv[3])
base_port = int(sys.argv[4])

# Mirrored order reduces first-order thermal/time drift bias without pretending
# the host is a laboratory-controlled environment.
session_order = ["off", "normal", "detailed", "detailed", "normal", "off"]
samples = {"off": [], "normal": [], "detailed": []}
request_body = json.dumps({
    "prompt": "AIR observes execution evidence.",
    "max_tokens": 24,
    "temperature": 0.0,
}).encode()

def wait_health(port, proc):
    url = f"http://127.0.0.1:{port}/health"
    for _ in range(160):
        if proc.poll() is not None:
            raise RuntimeError(f"server exited early rc={proc.returncode}")
        try:
            with urllib.request.urlopen(url, timeout=0.5) as response:
                if response.status == 200:
                    return
        except Exception:
            pass
        time.sleep(0.25)
    raise RuntimeError(f"server not healthy on port {port}")

def generate(port):
    request = urllib.request.Request(
        f"http://127.0.0.1:{port}/generate",
        data=request_body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=120) as response:
        result = json.loads(response.read().decode())
    return float(result["metrics"]["total_ms"])

for session_index, mode in enumerate(session_order):
    port = base_port + session_index
    log_path = out_dir / f"overhead-{session_index:02d}-{mode}.log"
    with log_path.open("wb") as log:
        proc = subprocess.Popen([
            server,
            "-m", model,
            "--backend", "cuda",
            "--device", "0",
            "--host", "127.0.0.1",
            "--port", str(port),
            "--no-manifest",
            "--execution-observation", mode,
            "--execution-span-capacity", "16384",
        ], stdout=log, stderr=subprocess.STDOUT)
        try:
            wait_health(port, proc)
            for _ in range(2):
                generate(port)
            session_samples = [generate(port) for _ in range(6)]
            samples[mode].extend(session_samples)

            with urllib.request.urlopen(
                f"http://127.0.0.1:{port}/timeline", timeout=5
            ) as response:
                timeline = json.loads(response.read().decode())
            (out_dir / f"overhead-{session_index:02d}-{mode}-timeline.json").write_text(
                json.dumps(timeline, indent=2) + "\n"
            )
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()

summary = {}
for mode, values in samples.items():
    summary[mode] = {
        "samples": len(values),
        "median_total_ms": statistics.median(values),
        "mean_total_ms": statistics.mean(values),
        "min_total_ms": min(values),
        "max_total_ms": max(values),
    }

base = summary["off"]["median_total_ms"]
for mode in ["normal", "detailed"]:
    delta = summary[mode]["median_total_ms"] - base
    summary[mode]["median_delta_ms_vs_off"] = delta
    summary[mode]["median_delta_percent_vs_off"] = (
        (delta / base) * 100.0 if base > 0 else None
    )

summary["method"] = {
    "session_order": session_order,
    "warmups_per_session": 2,
    "samples_per_session": 6,
    "note": "mirrored order reduces first-order thermal/time drift; this is measured evidence, not a laboratory causal estimate",
}

(out_dir / "observer-overhead.json").write_text(
    json.dumps(summary, indent=2) + "\n"
)

print("observer_overhead_measurement=PASS")
for mode in ["off", "normal", "detailed"]:
    row = summary[mode]
    print(f"{mode}_median_total_ms={row['median_total_ms']:.6f}")
    if mode != "off":
        print(
            f"{mode}_median_delta_percent_vs_off="
            f"{row['median_delta_percent_vs_off']:.3f}"
        )
PY
STAGE="timeline-validation"
python3 - "$OUT" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

def load(name):
    return json.loads((root / name).read_text())

def validate_common(timeline, expected_level):
    assert timeline["schema_version"] == 1
    assert timeline["level"] == expected_level
    assert timeline["clock"] == "steady_clock"
    assert timeline["time_unit"] == "nanoseconds_from_origin"
    assert timeline["origin_unix_ms"] > 0
    assert timeline["capacity"] > 0
    assert timeline["dropped_spans"] == 0
    spans = timeline["spans"]
    previous = 0
    for span in spans:
        seq = int(span["observation_sequence"])
        assert seq > previous
        previous = seq
        assert int(span["end_ns"]) >= int(span["start_ns"])
        assert int(span["participant_count"]) >= 1
    return spans

ref = load("reference-normal-timeline.json")
off = load("cuda-off-timeline.json")
normal = load("cuda-normal-timeline.json")
detailed = load("cuda-detailed-timeline.json")

ref_spans = validate_common(ref, "normal")
off_spans = validate_common(off, "off")
normal_spans = validate_common(normal, "normal")
detailed_spans = validate_common(detailed, "detailed")

assert off_spans == [], "off mode emitted spans"

ref_categories = {(s["scope"], s["category"], s["phase"]) for s in ref_spans}
assert ("service", "queue", "queue-wait") in ref_categories
assert any(s["scope"] == "service" and s["phase"] == "prefill" for s in ref_spans)
assert any(s["scope"] == "service" and s["phase"] == "decode" for s in ref_spans)
assert any(s["scope"] == "service" and s["category"] == "request" for s in ref_spans)
assert all(
    int(s["request_id"]) > 0 and int(s["sequence_id"]) > 0
    for s in ref_spans
    if int(s["participant_count"]) == 1
)

assert normal_spans, "normal CUDA mode emitted no service spans"
assert all(s["scope"] == "service" for s in normal_spans), (
    "normal mode unexpectedly emitted backend-level spans"
)

backend = [s for s in detailed_spans if s["scope"] == "backend"]
assert backend, "detailed CUDA mode emitted no backend spans"
assert any(s["category"] == "transfer" for s in backend), (
    "detailed CUDA timeline has no transfer evidence"
)
assert any(s["category"] == "synchronization" for s in backend), (
    "detailed CUDA timeline has no synchronization evidence"
)
assert any(str(s["phase"]).startswith("h2d-") for s in backend), (
    "detailed CUDA timeline has no H2D transfer evidence"
)
assert any(str(s["phase"]).startswith("d2h-") for s in backend), (
    "detailed CUDA timeline has no D2H transfer evidence"
)
assert all(int(s["request_id"]) > 0 for s in backend), (
    "backend CUDA observations lost request correlation"
)
assert all(int(s["sequence_id"]) > 0 for s in backend), (
    "backend CUDA observations lost sequence correlation"
)
assert all(s["backend"] == "cuda" for s in backend)

for response_name in [
    "reference-normal-response.json",
    "cuda-off-response.json",
    "cuda-normal-response.json",
    "cuda-detailed-response.json",
]:
    response = load(response_name)
    assert "metrics" in response
    assert int(response["metrics"]["generated_tokens"]) > 0
    assert float(response["metrics"]["total_ms"]) >= 0.0

overhead = load("observer-overhead.json")
for mode in ["off", "normal", "detailed"]:
    assert overhead[mode]["samples"] == 12
    assert overhead[mode]["median_total_ms"] > 0.0

print("prompt3_timeline_validation=PASS")
print("reference_spans=" + str(len(ref_spans)))
print("cuda_normal_spans=" + str(len(normal_spans)))
print("cuda_detailed_spans=" + str(len(detailed_spans)))
print("cuda_backend_spans=" + str(len(backend)))
print("cuda_transfer_spans=" + str(sum(s["category"] == "transfer" for s in backend)))
print("cuda_sync_spans=" + str(sum(s["category"] == "synchronization" for s in backend)))
print(
    "normal_overhead_percent="
    + f"{overhead['normal']['median_delta_percent_vs_off']:.3f}"
)
print(
    "detailed_overhead_percent="
    + f"{overhead['detailed']['median_delta_percent_vs_off']:.3f}"
)
PY

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 3 qualification mutated the source worktree." >&2
    git status --short >&2
    exit 4
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
echo "PROMPT3_EXECUTION_OBSERVATION=PASS"
echo "Evidence directory: $OUT"
echo "Observer overhead is measured evidence only; review observer-overhead.json before closing Prompt 3."
