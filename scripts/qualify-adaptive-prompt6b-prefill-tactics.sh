#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-}"
BASE_PORT="${2:-18680}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_P6B_OUT:-$HOME/Downloads/AIR-0.11-Prompt6B-Prefill-Tactics-$STAMP}"
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
    echo "PROMPT6B_PREFILL_TACTIC_CENSUS=FAIL" >&2
    echo "failed_stage=$STAGE" >&2
    echo "exit_code=$rc" >&2
    echo "Evidence directory: $OUT" >&2
    for log in "$OUT/preflight-terminal.txt" "$OUT/configure.log" "$OUT/build.log" "$OUT/tactic-census.log"; do
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
    echo "ERROR: Prompt 6B census requires a clean worktree." >&2
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

if command -v nvidia-smi >/dev/null; then
    nvidia-smi > "$OUT/nvidia-smi-before.txt" || true
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader         > "$OUT/compute-apps-before.txt" 2>/dev/null || true
fi

echo "=== ADAPTIVE CPU PREFLIGHT ==="
STAGE="cpu-preflight"
AIR_PREFLIGHT_OUT="$OUT/preflight-build"     bash "$ROOT/scripts/preflight-adaptive.sh"     2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== CUDA SERVER BUILD ==="
STAGE="cuda-configure"
cmake -S "$ROOT" -B "$BUILD"     -DCMAKE_BUILD_TYPE=Release     -DAIR_ENABLE_CUDA=ON     > "$OUT/configure.log" 2>&1

STAGE="cuda-build"
cmake --build "$BUILD" --target air-server -j"$(nproc)"     > "$OUT/build.log" 2>&1

echo
echo "=== PROMPT 6B BALANCED PREFILL-TACTIC CENSUS ==="
STAGE="balanced-prefill-tactic-census"
python3 - "$BUILD/air-server" "$MODEL" "$OUT" "$BASE_PORT" <<'PY'     2>&1 | tee "$OUT/tactic-census.log"
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

tactics = ["baseline", "reuse8", "dense-f32-cublas"]
rounds = [
    ["baseline", "reuse8", "dense-f32-cublas"],
    ["reuse8", "dense-f32-cublas", "baseline"],
    ["dense-f32-cublas", "baseline", "reuse8"],
]

sentence = (
    "AIR compares qualified physical implementations using the same semantic "
    "program, machine, scheduler, and deterministic output contract. "
)
prompt = sentence * 48

request_body = json.dumps({
    "prompt": prompt,
    "max_tokens": 2,
    "temperature": 0.0,
}).encode()

session_rows = []
request_rows = []
telemetry_rows = []
all_texts = []

def wait_health(port, proc):
    url = f"http://127.0.0.1:{port}/health"
    for _ in range(200):
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
    with urllib.request.urlopen(request, timeout=240) as response:
        return json.loads(response.read().decode())

def gpu_telemetry(round_index, position, tactic, phase):
    if not shutil.which("nvidia-smi"):
        return
    cmd = [
        "nvidia-smi",
        "--query-gpu=index,temperature.gpu,pstate,clocks.sm,clocks.mem,power.draw,utilization.gpu,memory.used,memory.free",
        "--format=csv,noheader,nounits",
    ]
    try:
        row = subprocess.check_output(cmd, text=True, timeout=5).strip().splitlines()[0]
        values = [part.strip() for part in row.split(",")]
        telemetry_rows.append({
            "round": round_index,
            "position": position,
            "tactic": tactic,
            "phase": phase,
            "gpu_index": values[0],
            "temperature_c": values[1],
            "pstate": values[2],
            "sm_clock_mhz": values[3],
            "memory_clock_mhz": values[4],
            "power_w": values[5],
            "utilization_percent": values[6],
            "memory_used_mib": values[7],
            "memory_free_mib": values[8],
        })
    except Exception as exc:
        telemetry_rows.append({
            "round": round_index,
            "position": position,
            "tactic": tactic,
            "phase": phase,
            "error": str(exc),
        })

def compute_apps():
    if not shutil.which("nvidia-smi"):
        return []
    try:
        out = subprocess.check_output([
            "nvidia-smi",
            "--query-compute-apps=pid,process_name,used_memory",
            "--format=csv,noheader,nounits",
        ], text=True, timeout=5)
        return [line.strip() for line in out.splitlines() if line.strip()]
    except Exception:
        return []

session_index = 0
for round_index, order in enumerate(rounds, start=1):
    for position, tactic in enumerate(order, start=1):
        port = base_port + session_index
        safe = tactic.replace("-", "_")
        log_path = out_dir / (
            f"session-{session_index:02d}-r{round_index}-p{position}-{safe}.log"
        )

        gpu_telemetry(round_index, position, tactic, "before")
        apps_before = compute_apps()

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
                "--prefill-quantum", "32",
                "--prefix-cache", "0",
                "--cuda-prefill-block-linear", tactic,
                "--cuda-decode-block-linear", "baseline",
                "--cuda-decode-output-linear", "baseline",
                "--cuda-prefill-attention", "baseline",
                "--execution-observation", "normal",
            ], stdout=log, stderr=subprocess.STDOUT)

            try:
                wait_health(port, proc)
                gpu_telemetry(round_index, position, tactic, "loaded")

                warmup = generate(port)
                warmup_runtime = get_json(port, "/runtime")
                selected = warmup_runtime["planner"]["prefill_block_quantized_linear"]
                normalized_expected = {
                    "baseline": "baseline",
                    "reuse8": "batch-reuse8",
                    "dense-f32-cublas": "dense-f32-cublas",
                }[tactic]
                if selected != normalized_expected:
                    raise RuntimeError(
                        f"selected prefill tactic mismatch requested={tactic} selected={selected}"
                    )
                if warmup["metrics"]["backend"] != "cuda":
                    raise RuntimeError("warmup did not execute on CUDA")
                if int(warmup["metrics"]["generated_tokens"]) == 0:
                    raise RuntimeError("warmup generated no tokens")

                gpu_telemetry(round_index, position, tactic, "warmed")

                results = [generate(port) for _ in range(4)]
                runtime = get_json(port, "/runtime")
                gpu_telemetry(round_index, position, tactic, "post-samples")

                if runtime["planner"]["prefill_block_quantized_linear"] != normalized_expected:
                    raise RuntimeError("runtime tactic changed during measured samples")

                warmup_text = warmup.get("text", "")
                texts = [result.get("text", "") for result in results]
                if any(text != warmup_text for text in texts):
                    raise RuntimeError(
                        f"deterministic output changed within tactic {tactic}"
                    )
                all_texts.append({
                    "tactic": tactic,
                    "round": round_index,
                    "warmup_text": warmup_text,
                    "measured_texts": texts,
                })

                rows = []
                for sample_index, result in enumerate(results):
                    metrics = result["metrics"]
                    row = {
                        "session_index": session_index,
                        "round": round_index,
                        "position": position,
                        "tactic": tactic,
                        "sample_index": sample_index,
                        "text": result.get("text", ""),
                        "prompt_tokens": int(metrics["prompt_tokens"]),
                        "generated_tokens": int(metrics["generated_tokens"]),
                        "ttft_ms": float(metrics["ttft_ms"]),
                        "prefill_ms": float(metrics["prefill_ms"]),
                        "total_ms": float(metrics["total_ms"]),
                        "prefill_tokens_per_second": float(
                            metrics["prefill_tokens_per_second"]
                        ),
                        "plan_preparation_ms": float(metrics["plan_preparation_ms"]),
                        "plan_preparation_bytes": int(metrics["plan_preparation_bytes"]),
                    }
                    rows.append(row)
                    request_rows.append(row)

                prompt_tokens = {row["prompt_tokens"] for row in rows}
                generated_tokens = {row["generated_tokens"] for row in rows}
                if len(prompt_tokens) != 1 or len(generated_tokens) != 1:
                    raise RuntimeError("token counts changed within tactic session")

                hot_prep_ms = [row["plan_preparation_ms"] for row in rows]
                session_rows.append({
                    "session_index": session_index,
                    "round": round_index,
                    "position": position,
                    "tactic": tactic,
                    "sample_count": len(rows),
                    "prompt_tokens": next(iter(prompt_tokens)),
                    "generated_tokens": next(iter(generated_tokens)),
                    "warmup_text": warmup_text,
                    "warmup_plan_preparation_ms": float(
                        warmup["metrics"]["plan_preparation_ms"]
                    ),
                    "warmup_plan_preparation_bytes": int(
                        warmup["metrics"]["plan_preparation_bytes"]
                    ),
                    "median_hot_plan_preparation_ms": statistics.median(hot_prep_ms),
                    "current_prepared_artifact_bytes": int(
                        runtime["current_prepared_artifact_bytes"]
                    ),
                    "current_device_bytes": int(runtime["current_device_bytes"]),
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
                    "compute_apps_before": apps_before,
                    "compute_apps_after": compute_apps(),
                })

                (out_dir / f"session-{session_index:02d}-{safe}-warmup.json").write_text(
                    json.dumps(warmup, indent=2) + "\n"
                )
                (out_dir / f"session-{session_index:02d}-{safe}-responses.json").write_text(
                    json.dumps(results, indent=2) + "\n"
                )
                (out_dir / f"session-{session_index:02d}-{safe}-runtime.json").write_text(
                    json.dumps(runtime, indent=2) + "\n"
                )
            finally:
                proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()

        gpu_telemetry(round_index, position, tactic, "after")
        session_index += 1
        time.sleep(2.0)

reference_text = all_texts[0]["warmup_text"]
for entry in all_texts:
    if entry["warmup_text"] != reference_text:
        raise RuntimeError(
            "deterministic generated text differs across prefill tactics"
        )
    if any(text != reference_text for text in entry["measured_texts"]):
        raise RuntimeError(
            "measured deterministic text differs across prefill tactics"
        )

summary = {
    "method": {
        "rounds": rounds,
        "warmups_per_session": 1,
        "samples_per_session": 4,
        "sessions_per_tactic": 3,
        "samples_per_tactic": 12,
        "prefill_quantum": 32,
        "token_budget": 256,
        "max_active": 1,
        "prefix_cache_entries": 0,
        "prefill_attention": "baseline",
        "decode_block": "baseline",
        "decode_output": "baseline",
        "temperature": 0.0,
        "max_tokens": 2,
        "analysis": "median of per-session medians; each tactic occupies each ordinal position once",
    },
    "deterministic_text": reference_text,
    "tactics": {},
}

for tactic in tactics:
    rows = [row for row in session_rows if row["tactic"] == tactic]
    if len(rows) != 3:
        raise RuntimeError(f"missing sessions for tactic {tactic}")
    summary["tactics"][tactic] = {
        "sessions": 3,
        "samples": 12,
        "prompt_tokens": rows[0]["prompt_tokens"],
        "generated_tokens": rows[0]["generated_tokens"],
        "median_ttft_ms": statistics.median(
            row["median_ttft_ms"] for row in rows
        ),
        "median_prefill_ms": statistics.median(
            row["median_prefill_ms"] for row in rows
        ),
        "median_total_ms": statistics.median(
            row["median_total_ms"] for row in rows
        ),
        "median_prefill_tokens_per_second": statistics.median(
            row["median_prefill_tokens_per_second"] for row in rows
        ),
        "median_warmup_plan_preparation_ms": statistics.median(
            row["warmup_plan_preparation_ms"] for row in rows
        ),
        "median_warmup_plan_preparation_bytes": statistics.median(
            row["warmup_plan_preparation_bytes"] for row in rows
        ),
        "median_hot_plan_preparation_ms": statistics.median(
            row["median_hot_plan_preparation_ms"] for row in rows
        ),
        "median_current_prepared_artifact_bytes": statistics.median(
            row["current_prepared_artifact_bytes"] for row in rows
        ),
        "median_current_device_bytes": statistics.median(
            row["current_device_bytes"] for row in rows
        ),
        "session_rows": rows,
    }

baseline = summary["tactics"]["baseline"]
for tactic in ["reuse8", "dense-f32-cublas"]:
    row = summary["tactics"][tactic]
    for metric in [
        "median_ttft_ms",
        "median_prefill_ms",
        "median_total_ms",
    ]:
        base = baseline[metric]
        value = row[metric]
        row[metric + "_delta_percent_vs_baseline"] = (
            (value - base) / base * 100.0 if base else None
        )
    base_rate = baseline["median_prefill_tokens_per_second"]
    rate = row["median_prefill_tokens_per_second"]
    row["prefill_tokens_per_second_delta_percent_vs_baseline"] = (
        (rate - base_rate) / base_rate * 100.0 if base_rate else None
    )

(out_dir / "prompt6b-prefill-tactic-summary.json").write_text(
    json.dumps(summary, indent=2) + "\n"
)
(out_dir / "prompt6b-prefill-tactic-sessions.json").write_text(
    json.dumps(session_rows, indent=2) + "\n"
)
(out_dir / "prompt6b-prefill-tactic-requests.json").write_text(
    json.dumps(request_rows, indent=2) + "\n"
)
(out_dir / "prompt6b-prefill-tactic-telemetry.json").write_text(
    json.dumps(telemetry_rows, indent=2) + "\n"
)
(out_dir / "prompt6b-deterministic-text.json").write_text(
    json.dumps(all_texts, indent=2) + "\n"
)

print("PROMPT6B_PREFILL_TACTIC_CENSUS=PASS")
for tactic in tactics:
    row = summary["tactics"][tactic]
    print(
        f"{tactic}: "
        f"ttft_ms={row['median_ttft_ms']:.6f} "
        f"prefill_ms={row['median_prefill_ms']:.6f} "
        f"prefill_tok_s={row['median_prefill_tokens_per_second']:.3f} "
        f"warmup_prep_ms={row['median_warmup_plan_preparation_ms']:.6f} "
        f"prep_bytes={int(row['median_warmup_plan_preparation_bytes'])} "
        f"artifact_bytes={int(row['median_current_prepared_artifact_bytes'])}"
    )

for tactic in ["reuse8", "dense-f32-cublas"]:
    row = summary["tactics"][tactic]
    print(
        f"{tactic}_ttft_delta_percent_vs_baseline="
        f"{row['median_ttft_ms_delta_percent_vs_baseline']:.3f}"
    )
    print(
        f"{tactic}_prefill_delta_percent_vs_baseline="
        f"{row['median_prefill_ms_delta_percent_vs_baseline']:.3f}"
    )
    print(
        f"{tactic}_prefill_tok_s_delta_percent_vs_baseline="
        f"{row['prefill_tokens_per_second_delta_percent_vs_baseline']:.3f}"
    )

print("deterministic_output_equality=PASS")
print(f"Evidence directory: {out_dir}")
PY

STAGE="worktree-cleanliness"
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: Prompt 6B census mutated the source worktree." >&2
    git status --short >&2
    exit 4
fi

if command -v nvidia-smi >/dev/null; then
    nvidia-smi > "$OUT/nvidia-smi-after.txt" || true
    nvidia-smi         --query-compute-apps=pid,process_name,used_memory         --format=csv,noheader         > "$OUT/compute-apps-after.txt" 2>/dev/null || true
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
echo "PROMPT6B_PREFILL_TACTIC_CENSUS=PASS"
echo "Evidence directory: $OUT"
