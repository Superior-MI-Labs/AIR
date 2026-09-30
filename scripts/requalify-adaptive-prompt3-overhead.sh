#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18420}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P3_OVERHEAD_OUT:-$HOME/Downloads/AIR-0.11-Prompt3-Overhead-$STAMP}"
BUILD="$OUT/build-cuda"
STAGE="initialization"

if [[ -z "$MODEL" ]]; then
    echo "usage: $0 /path/to/model.gguf [base-port]" >&2
    exit 2
fi

mkdir -p "$OUT"
cd "$ROOT"

on_error() {
    local rc=$?
    set +e
    echo >&2
    echo "PROMPT3_OVERHEAD_FALSIFICATION=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in "$OUT/configure.log" "$OUT/build.log" "$OUT/overhead-run.log"; do
        if [[ -s "$log" ]]; then
            echo >&2
            echo "=== tail: $(basename "$log") ===" >&2
            tail -n 160 "$log" >&2
        fi
    done
    exit "$rc"
}
trap on_error ERR

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 3 overhead falsification requires a clean worktree." >&2
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

echo "=== CUDA BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=ON     > "$OUT/configure.log" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" --target air-server -j"$(nproc)"     > "$OUT/build.log" 2>&1

echo
echo "=== BALANCED OBSERVER OVERHEAD FALSIFICATION ==="
STAGE="balanced-overhead"
python3 - "$BUILD/air-server" "$MODEL" "$OUT" "$BASE_PORT" <<'PY' \
    2>&1 | tee "$OUT/overhead-run.log"
import csv
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

# 3x3 balanced Latin square: each observation mode occupies each ordinal
# position exactly once. This directly attacks the previous experiment's
# mode-position / thermal confound.
rounds = [
    ["off", "normal", "detailed"],
    ["normal", "detailed", "off"],
    ["detailed", "off", "normal"],
]
request_body = json.dumps({
    "prompt": "AIR observes execution evidence.",
    "max_tokens": 24,
    "temperature": 0.0,
}).encode()

samples = {mode: [] for mode in ["off", "normal", "detailed"]}
session_rows = []
telemetry_rows = []

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

def gpu_telemetry(round_index, position, mode, phase):
    if not shutil_which("nvidia-smi"):
        return
    query = [
        "nvidia-smi",
        "--query-gpu=index,temperature.gpu,pstate,clocks.sm,clocks.mem,power.draw,utilization.gpu,memory.used",
        "--format=csv,noheader,nounits",
    ]
    try:
        row = subprocess.check_output(query, text=True, timeout=5).strip().splitlines()[0]
        values = [part.strip() for part in row.split(",")]
        telemetry_rows.append({
            "round": round_index,
            "position": position,
            "mode": mode,
            "phase": phase,
            "gpu_index": values[0],
            "temperature_c": values[1],
            "pstate": values[2],
            "sm_clock_mhz": values[3],
            "memory_clock_mhz": values[4],
            "power_w": values[5],
            "utilization_percent": values[6],
            "memory_used_mib": values[7],
        })
    except Exception as exc:
        telemetry_rows.append({
            "round": round_index,
            "position": position,
            "mode": mode,
            "phase": phase,
            "error": str(exc),
        })

def shutil_which(name):
    import shutil
    return shutil.which(name)

session_index = 0
for round_index, order in enumerate(rounds, start=1):
    for position, mode in enumerate(order, start=1):
        port = base_port + session_index
        log_path = out_dir / f"session-{session_index:02d}-r{round_index}-p{position}-{mode}.log"
        gpu_telemetry(round_index, position, mode, "before")
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
                gpu_telemetry(round_index, position, mode, "loaded")
                for _ in range(2):
                    generate(port)
                gpu_telemetry(round_index, position, mode, "warmed")
                values = [generate(port) for _ in range(6)]
                samples[mode].extend(values)
                gpu_telemetry(round_index, position, mode, "post-samples")

                with urllib.request.urlopen(
                    f"http://127.0.0.1:{port}/timeline", timeout=5
                ) as response:
                    timeline = json.loads(response.read().decode())

                (out_dir / f"session-{session_index:02d}-{mode}-timeline.json").write_text(
                    json.dumps(timeline, indent=2) + "\n"
                )

                session_rows.append({
                    "session_index": session_index,
                    "round": round_index,
                    "position": position,
                    "mode": mode,
                    "sample_count": len(values),
                    "median_total_ms": statistics.median(values),
                    "mean_total_ms": statistics.mean(values),
                    "min_total_ms": min(values),
                    "max_total_ms": max(values),
                    "timeline_spans": len(timeline["spans"]),
                    "dropped_spans": int(timeline["dropped_spans"]),
                })
            finally:
                proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
        gpu_telemetry(round_index, position, mode, "after")
        session_index += 1
        time.sleep(2.0)

session_medians = {
    mode: [row["median_total_ms"] for row in session_rows if row["mode"] == mode]
    for mode in samples
}

summary = {}
for mode, values in samples.items():
    summary[mode] = {
        "samples": len(values),
        "sessions": len(session_medians[mode]),
        "sample_median_total_ms": statistics.median(values),
        "sample_mean_total_ms": statistics.mean(values),
        "session_medians_ms": session_medians[mode],
        "median_of_session_medians_ms": statistics.median(session_medians[mode]),
        "min_total_ms": min(values),
        "max_total_ms": max(values),
    }

base = summary["off"]["median_of_session_medians_ms"]
for mode in ["normal", "detailed"]:
    value = summary[mode]["median_of_session_medians_ms"]
    delta = value - base
    summary[mode]["median_session_delta_ms_vs_off"] = delta
    summary[mode]["median_session_delta_percent_vs_off"] = (
        delta / base * 100.0 if base > 0 else None
    )

normal_base = summary["normal"]["median_of_session_medians_ms"]
detailed_value = summary["detailed"]["median_of_session_medians_ms"]
summary["detailed"]["median_session_delta_ms_vs_normal"] = (
    detailed_value - normal_base
)
summary["detailed"]["median_session_delta_percent_vs_normal"] = (
    (detailed_value - normal_base) / normal_base * 100.0
    if normal_base > 0 else None
)

summary["method"] = {
    "rounds": rounds,
    "warmups_per_session": 2,
    "samples_per_session": 6,
    "sessions_per_mode": 3,
    "samples_per_mode": 18,
    "inter_session_sleep_seconds": 2.0,
    "analysis": "median of per-session medians; mode occupies each ordinal position once",
    "purpose": "falsify mode-position and first-order thermal drift as causes of the initial apparent overhead",
}

(out_dir / "observer-overhead-v2.json").write_text(
    json.dumps(summary, indent=2) + "\n"
)
(out_dir / "observer-overhead-v2-sessions.json").write_text(
    json.dumps(session_rows, indent=2) + "\n"
)
(out_dir / "observer-overhead-v2-telemetry.json").write_text(
    json.dumps(telemetry_rows, indent=2) + "\n"
)

assert all(row["dropped_spans"] == 0 for row in session_rows)
off_timelines = [row["timeline_spans"] for row in session_rows if row["mode"] == "off"]
normal_timelines = [row["timeline_spans"] for row in session_rows if row["mode"] == "normal"]
detailed_timelines = [row["timeline_spans"] for row in session_rows if row["mode"] == "detailed"]
assert all(value == 0 for value in off_timelines)
assert all(value > 0 for value in normal_timelines)
assert all(value > 0 for value in detailed_timelines)

print("PROMPT3_OVERHEAD_FALSIFICATION=PASS")
for mode in ["off", "normal", "detailed"]:
    row = summary[mode]
    print(f"{mode}_median_of_session_medians_ms={row['median_of_session_medians_ms']:.6f}")
if summary["normal"].get("median_session_delta_percent_vs_off") is not None:
    print(
        "normal_delta_percent_vs_off="
        f"{summary['normal']['median_session_delta_percent_vs_off']:.3f}"
    )
if summary["detailed"].get("median_session_delta_percent_vs_off") is not None:
    print(
        "detailed_delta_percent_vs_off="
        f"{summary['detailed']['median_session_delta_percent_vs_off']:.3f}"
    )
if summary["detailed"].get("median_session_delta_percent_vs_normal") is not None:
    print(
        "detailed_delta_percent_vs_normal="
        f"{summary['detailed']['median_session_delta_percent_vs_normal']:.3f}"
    )
print(f"Evidence directory: {out_dir}")
PY

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: overhead falsification mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

STAGE="evidence-checksums"
python3 - "$OUT" <<'PY'
import hashlib
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
files = [p for p in root.rglob("*") if p.is_file() and p.name != "SHA256SUMS.txt"]
with (root / "SHA256SUMS.txt").open("w") as out:
    for p in sorted(files):
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        out.write(f"{h}  {p.relative_to(root)}\n")
PY

trap - ERR

echo
echo "PROMPT3_OVERHEAD_REMEASURE=PASS"
echo "Evidence directory: $OUT"
