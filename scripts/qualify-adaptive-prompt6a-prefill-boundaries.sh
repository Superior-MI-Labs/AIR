#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18620}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P6A_OUT:-$HOME/Downloads/AIR-0.11-Prompt6A-Prefill-$STAMP}"
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
    echo "PROMPT6A_PREFILL_BOUNDARY_CENSUS=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in "$OUT/preflight-terminal.txt" "$OUT/configure.log" "$OUT/build.log" "$OUT/prefill-census.log"; do
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
    echo "ERROR: Prompt 6A census requires a clean worktree." >&2
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
AIR_PREFLIGHT_OUT="$OUT/preflight-build" \
    bash "$ROOT/scripts/preflight-adaptive.sh" \
    2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== CUDA SERVER BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAIR_ENABLE_CUDA=ON \
    > "$OUT/configure.log" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" --target air-server -j"$(nproc)" \
    > "$OUT/build.log" 2>&1

echo
echo "=== PROMPT 6A BALANCED PREFILL-BOUNDARY CENSUS ==="
STAGE="balanced-prefill-census"
python3 - "$BUILD/air-server" "$MODEL" "$OUT" "$BASE_PORT" <<'PY' \
    2>&1 | tee "$OUT/prefill-census.log"
import json
import pathlib
import shutil
import statistics
import subprocess
import sys
import time
import urllib.request

server = sys.argv[1]
model = sys.argv[2]
out_dir = pathlib.Path(sys.argv[3])
base_port = int(sys.argv[4])

rounds = [
    [32, 64, 128],
    [64, 128, 32],
    [128, 32, 64],
]

# Long enough to force multiple outputless chunks at every tested quantum while
# staying comfortably within the qualified model context.
sentence = (
    "AIR measures physical scheduling boundaries before changing execution "
    "ownership or synchronization semantics. "
)
prompt = sentence * 96

request_body = json.dumps({
    "prompt": prompt,
    "max_tokens": 1,
    "temperature": 0.0,
}).encode()

session_rows = []
request_rows = []
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

def get_json(port, path):
    with urllib.request.urlopen(
        f"http://127.0.0.1:{port}{path}", timeout=10
    ) as response:
        return json.loads(response.read().decode())

def generate(port):
    request = urllib.request.Request(
        f"http://127.0.0.1:{port}/generate",
        data=request_body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=180) as response:
        return json.loads(response.read().decode())

def gpu_telemetry(round_index, position, quantum, phase):
    if not shutil.which("nvidia-smi"):
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
            "quantum": quantum,
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
            "quantum": quantum,
            "phase": phase,
            "error": str(exc),
        })

def span_ms(span):
    return (int(span["end_ns"]) - int(span["start_ns"])) / 1_000_000.0

def summarize_request(result, timeline):
    metrics = result["metrics"]
    rid = int(metrics["request_id"])
    spans = [s for s in timeline["spans"] if int(s["request_id"]) == rid]

    def selected(category=None, phase=None):
        rows = spans
        if category is not None:
            rows = [s for s in rows if s["category"] == category]
        if phase is not None:
            rows = [s for s in rows if s["phase"] == phase]
        return rows

    outputless = selected("synchronization", "cuda-stream-wait-outputless-prefill")
    greedy_wait = selected("synchronization", "cuda-stream-wait-greedy")
    token_h2d = selected("transfer", "h2d-prefill-token-enqueue")
    prefill_calls = selected("backend-call", "prefill")

    prompt_tokens = int(metrics["prompt_tokens"])
    prefill_ms = float(metrics["prefill_ms"])

    return {
        "request_id": rid,
        "sequence_id": int(metrics["sequence_id"]),
        "prompt_tokens": prompt_tokens,
        "generated_tokens": int(metrics["generated_tokens"]),
        "ttft_ms": float(metrics["ttft_ms"]),
        "prefill_ms": prefill_ms,
        "total_ms": float(metrics["total_ms"]),
        "prefill_tokens_per_second": (
            prompt_tokens * 1000.0 / prefill_ms if prefill_ms > 0.0 else 0.0
        ),
        "outputless_sync_count": len(outputless),
        "outputless_sync_host_wait_ms": sum(span_ms(s) for s in outputless),
        "greedy_sync_count": len(greedy_wait),
        "greedy_sync_host_wait_ms": sum(span_ms(s) for s in greedy_wait),
        "h2d_token_enqueue_count": len(token_h2d),
        "h2d_token_bytes": sum(int(s["work_units"]) for s in token_h2d),
        "h2d_token_enqueue_host_ms": sum(span_ms(s) for s in token_h2d),
        "prefill_backend_call_count": len(prefill_calls),
        "prefill_backend_call_host_ms": sum(span_ms(s) for s in prefill_calls),
    }

session_index = 0
for round_index, order in enumerate(rounds, start=1):
    for position, quantum in enumerate(order, start=1):
        port = base_port + session_index
        log_path = out_dir / (
            f"session-{session_index:02d}-r{round_index}-p{position}-q{quantum}.log"
        )
        gpu_telemetry(round_index, position, quantum, "before")

        with log_path.open("wb") as log:
            proc = subprocess.Popen([
                server,
                "-m", model,
                "--backend", "cuda",
                "--device", "0",
                "--host", "127.0.0.1",
                "--port", str(port),
                "--no-manifest",
                "--max-active", "1",
                "--token-budget", "256",
                "--prefill-quantum", str(quantum),
                "--prefix-cache", "0",
                "--execution-observation", "detailed",
                "--execution-span-capacity", "32768",
            ], stdout=log, stderr=subprocess.STDOUT)

            try:
                wait_health(port, proc)
                gpu_telemetry(round_index, position, quantum, "loaded")

                # Long warmup ensures model kernels, KV pool growth, and this
                # prompt geometry are warm before measured requests.
                warmup = generate(port)
                if int(warmup["metrics"]["generated_tokens"]) != 1:
                    raise RuntimeError("long warmup did not generate exactly one token")

                gpu_telemetry(round_index, position, quantum, "warmed")

                results = [generate(port) for _ in range(6)]
                timeline = get_json(port, "/timeline")
                graphs = get_json(port, "/execution-graphs")

                if int(timeline["dropped_spans"]) != 0:
                    raise RuntimeError("detailed timeline dropped spans")
                if int(graphs["dropped_graphs"]) != 0:
                    raise RuntimeError("execution graph observation dropped graphs")
                if int(graphs["derivation_failures"]) != 0:
                    raise RuntimeError("execution graph derivation failed")

                rows = [summarize_request(result, timeline) for result in results]
                for row in rows:
                    row.update({
                        "session_index": session_index,
                        "round": round_index,
                        "position": position,
                        "quantum": quantum,
                    })
                    request_rows.append(row)

                prompt_tokens = {row["prompt_tokens"] for row in rows}
                if len(prompt_tokens) != 1:
                    raise RuntimeError("tokenization changed inside one session")
                if min(prompt_tokens) <= 128:
                    raise RuntimeError(
                        f"prompt too short for boundary census: {prompt_tokens}"
                    )

                expected_max_chunks = (
                    next(iter(prompt_tokens)) + quantum - 1
                ) // quantum
                if any(row["outputless_sync_count"] == 0 for row in rows):
                    raise RuntimeError(
                        "long prompt did not exercise outputless prefill synchronization"
                    )

                session_rows.append({
                    "session_index": session_index,
                    "round": round_index,
                    "position": position,
                    "quantum": quantum,
                    "sample_count": len(rows),
                    "prompt_tokens": next(iter(prompt_tokens)),
                    "expected_max_chunks_from_quantum": expected_max_chunks,
                    "median_ttft_ms": statistics.median(
                        row["ttft_ms"] for row in rows
                    ),
                    "median_prefill_ms": statistics.median(
                        row["prefill_ms"] for row in rows
                    ),
                    "median_total_ms": statistics.median(
                        row["total_ms"] for row in rows
                    ),
                    "median_prefill_tokens_per_second": statistics.median(
                        row["prefill_tokens_per_second"] for row in rows
                    ),
                    "median_outputless_sync_count": statistics.median(
                        row["outputless_sync_count"] for row in rows
                    ),
                    "median_outputless_sync_host_wait_ms": statistics.median(
                        row["outputless_sync_host_wait_ms"] for row in rows
                    ),
                    "median_greedy_sync_host_wait_ms": statistics.median(
                        row["greedy_sync_host_wait_ms"] for row in rows
                    ),
                    "median_h2d_token_enqueue_count": statistics.median(
                        row["h2d_token_enqueue_count"] for row in rows
                    ),
                    "median_h2d_token_bytes": statistics.median(
                        row["h2d_token_bytes"] for row in rows
                    ),
                    "median_h2d_token_enqueue_host_ms": statistics.median(
                        row["h2d_token_enqueue_host_ms"] for row in rows
                    ),
                    "median_prefill_backend_call_count": statistics.median(
                        row["prefill_backend_call_count"] for row in rows
                    ),
                    "median_prefill_backend_call_host_ms": statistics.median(
                        row["prefill_backend_call_host_ms"] for row in rows
                    ),
                    "timeline_spans": len(timeline["spans"]),
                    "graph_observations": len(graphs["observations"]),
                })

                (out_dir / f"session-{session_index:02d}-q{quantum}-timeline.json").write_text(
                    json.dumps(timeline, indent=2) + "\n"
                )
                (out_dir / f"session-{session_index:02d}-q{quantum}-graphs.json").write_text(
                    json.dumps(graphs, indent=2) + "\n"
                )
                (out_dir / f"session-{session_index:02d}-q{quantum}-responses.json").write_text(
                    json.dumps(results, indent=2) + "\n"
                )
                gpu_telemetry(round_index, position, quantum, "post-samples")
            finally:
                proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()

        gpu_telemetry(round_index, position, quantum, "after")
        session_index += 1
        time.sleep(2.0)

metrics = [
    "median_ttft_ms",
    "median_prefill_ms",
    "median_total_ms",
    "median_prefill_tokens_per_second",
    "median_outputless_sync_count",
    "median_outputless_sync_host_wait_ms",
    "median_greedy_sync_host_wait_ms",
    "median_h2d_token_enqueue_count",
    "median_h2d_token_bytes",
    "median_h2d_token_enqueue_host_ms",
    "median_prefill_backend_call_count",
    "median_prefill_backend_call_host_ms",
]

summary = {
    "method": {
        "rounds": rounds,
        "warmups_per_session": 1,
        "samples_per_session": 6,
        "sessions_per_quantum": 3,
        "samples_per_quantum": 18,
        "token_budget": 256,
        "max_active": 1,
        "prefix_cache_entries": 0,
        "temperature": 0.0,
        "max_tokens": 1,
        "observation": "detailed",
        "analysis": "median of per-session medians; each quantum occupies each ordinal position once",
        "warning": (
            "synchronization host-wait spans include outstanding GPU work and "
            "are nested inside backend-call/request timing; they are not additive kernel timings"
        ),
    },
    "quantums": {},
}

for quantum in [32, 64, 128]:
    rows = [row for row in session_rows if row["quantum"] == quantum]
    if len(rows) != 3:
        raise RuntimeError(f"missing sessions for quantum {quantum}")
    entry = {
        "sessions": 3,
        "samples": 18,
        "prompt_tokens": rows[0]["prompt_tokens"],
        "session_rows": rows,
    }
    for metric in metrics:
        entry[metric] = statistics.median(row[metric] for row in rows)
    summary["quantums"][str(quantum)] = entry

baseline = summary["quantums"]["32"]
for quantum in [64, 128]:
    entry = summary["quantums"][str(quantum)]
    for metric in [
        "median_ttft_ms",
        "median_prefill_ms",
        "median_total_ms",
        "median_outputless_sync_host_wait_ms",
        "median_prefill_backend_call_host_ms",
    ]:
        base = baseline[metric]
        value = entry[metric]
        entry[metric + "_delta_vs_q32"] = value - base
        entry[metric + "_delta_percent_vs_q32"] = (
            (value - base) / base * 100.0 if base else None
        )
    base_rate = baseline["median_prefill_tokens_per_second"]
    rate = entry["median_prefill_tokens_per_second"]
    entry["prefill_tokens_per_second_delta_percent_vs_q32"] = (
        (rate - base_rate) / base_rate * 100.0 if base_rate else None
    )

(out_dir / "prompt6a-prefill-boundary-summary.json").write_text(
    json.dumps(summary, indent=2) + "\n"
)
(out_dir / "prompt6a-prefill-boundary-sessions.json").write_text(
    json.dumps(session_rows, indent=2) + "\n"
)
(out_dir / "prompt6a-prefill-boundary-requests.json").write_text(
    json.dumps(request_rows, indent=2) + "\n"
)
(out_dir / "prompt6a-prefill-boundary-telemetry.json").write_text(
    json.dumps(telemetry_rows, indent=2) + "\n"
)

print("PROMPT6A_PREFILL_BOUNDARY_CENSUS=PASS")
for quantum in [32, 64, 128]:
    row = summary["quantums"][str(quantum)]
    print(
        f"q{quantum}_ttft_ms={row['median_ttft_ms']:.6f} "
        f"prefill_ms={row['median_prefill_ms']:.6f} "
        f"prefill_tok_s={row['median_prefill_tokens_per_second']:.3f} "
        f"outputless_syncs={row['median_outputless_sync_count']:.1f} "
        f"outputless_wait_ms={row['median_outputless_sync_host_wait_ms']:.6f}"
    )

for quantum in [64, 128]:
    row = summary["quantums"][str(quantum)]
    print(
        f"q{quantum}_ttft_delta_percent_vs_q32="
        f"{row['median_ttft_ms_delta_percent_vs_q32']:.3f}"
    )
    print(
        f"q{quantum}_prefill_delta_percent_vs_q32="
        f"{row['median_prefill_ms_delta_percent_vs_q32']:.3f}"
    )
    print(
        f"q{quantum}_prefill_tok_s_delta_percent_vs_q32="
        f"{row['prefill_tokens_per_second_delta_percent_vs_q32']:.3f}"
    )
    print(
        f"q{quantum}_outputless_wait_delta_percent_vs_q32="
        f"{row['median_outputless_sync_host_wait_ms_delta_percent_vs_q32']:.3f}"
    )

print(f"Evidence directory: {out_dir}")
PY

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 6A census mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

if command -v nvidia-smi >/dev/null; then
    nvidia-smi \
        --query-gpu=index,name,uuid,memory.total,memory.free,driver_version,pstate,temperature.gpu,power.draw,clocks.sm,clocks.mem \
        --format=csv,noheader \
        > "$OUT/nvidia-smi-final.txt" || true
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
echo "PROMPT6A_PREFILL_BOUNDARY_CENSUS=PASS"
echo "Evidence directory: $OUT"
