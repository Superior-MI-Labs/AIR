#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import math
import pathlib
import shutil
import statistics
import subprocess
import sys
import time

T975 = {
    1: 12.706, 2: 4.303, 3: 3.182, 4: 2.776, 5: 2.571,
    6: 2.447, 7: 2.365, 8: 2.306, 9: 2.262, 10: 2.228,
}


def load(path: pathlib.Path):
    return json.loads(path.read_text())


def stats(values):
    xs = [float(x) for x in values if math.isfinite(float(x))]
    if not xs:
        return {"mean": 0.0, "stdev": 0.0, "half_width": 0.0}
    mean = statistics.fmean(xs)
    if len(xs) == 1:
        return {"mean": mean, "stdev": 0.0, "half_width": 0.0}
    stdev = statistics.stdev(xs)
    half = T975.get(len(xs) - 1, 1.96) * stdev / math.sqrt(len(xs))
    return {"mean": mean, "stdev": stdev, "half_width": half}


def run(cmd, *, log: pathlib.Path | None = None):
    print("[6C] " + " ".join(str(x) for x in cmd), flush=True)
    if log:
        with log.open("w") as fh:
            subprocess.run(cmd, check=True, stdout=fh, stderr=subprocess.STDOUT)
    else:
        subprocess.run(cmd, check=True)


def capture(cmd):
    return subprocess.check_output(cmd, text=True).strip()


def token_count(cli: pathlib.Path, model: pathlib.Path, text: str) -> int:
    line = capture([str(cli), "tokenize", str(model), text])
    if not line.startswith("tokens("):
        raise RuntimeError("unexpected air-cli tokenize output")
    return int(line.split("(", 1)[1].split(")", 1)[0])


def sample_gpu():
    if not shutil.which("nvidia-smi"):
        return None
    try:
        line = capture([
            "nvidia-smi",
            "--query-gpu=memory.used,memory.free,temperature.gpu,power.draw,utilization.gpu,clocks.sm,pstate",
            "--format=csv,noheader,nounits",
        ]).splitlines()[0]
        parts = [x.strip() for x in line.split(",")]
        return {
            "unix_s": time.time(),
            "used_mib": float(parts[0]),
            "free_mib": float(parts[1]),
            "temp_c": float(parts[2]),
            "power_w": float(parts[3]),
            "util_percent": float(parts[4]),
            "sm_clock_mhz": float(parts[5]),
            "pstate": parts[6],
        }
    except Exception as exc:
        return {"unix_s": time.time(), "error": str(exc)}


def run_probe(
    probe: pathlib.Path,
    model: pathlib.Path,
    manifest: pathlib.Path,
    medium: pathlib.Path,
    small: pathlib.Path,
    scenario: str,
    output: pathlib.Path,
    log: pathlib.Path,
    telemetry: pathlib.Path,
    *,
    cycles: int | None = None,
    allow_unknown_eviction: bool = False,
):
    cmd = [
        str(probe), "-m", str(model),
        "--manifest", str(manifest),
        "--medium-prompt-file", str(medium),
        "--small-prompt-file", str(small),
        "--scenario", scenario,
        "--horizon-tokens", "0",
        "--output", str(output),
    ]
    if cycles is not None:
        cmd += ["--cycles", str(cycles)]
    if allow_unknown_eviction:
        cmd.append("--qualification-allow-unknown-eviction")

    print("[6C] probe " + output.stem, flush=True)
    samples = []
    with log.open("w") as fh:
        proc = subprocess.Popen(cmd, stdout=fh, stderr=subprocess.STDOUT)
        while proc.poll() is None:
            sample = sample_gpu()
            if sample is not None:
                samples.append(sample)
            time.sleep(0.2)
        rc = proc.wait()
    telemetry.write_text(json.dumps(samples, indent=2) + "\n")
    if rc != 0:
        raise subprocess.CalledProcessError(rc, cmd)


def strategy(
    sid,
    workload,
    prompt_tokens,
    tactic,
    perf,
    *,
    dense=False,
):
    return {
        "workload": workload,
        "strategy_id": sid,
        "strict_qualified": True,
        "region_min_prompt_tokens": prompt_tokens,
        "region_max_prompt_tokens": prompt_tokens,
        "region_min_active_sequences": 1,
        "region_max_active_sequences": 1,
        "backend": "cuda",
        "prefill_quantum_tokens": 32,
        "prefill_block_quantized_linear": tactic,
        "decode_block_quantized_linear": "baseline",
        "decode_output_quantized_linear": "baseline",
        "prefill_attention": "baseline",
        "decode_attention": "baseline",
        "kv_page_tokens": perf["kv_page_tokens"],
        "samples": perf["samples"],
        "p50_ttft_ms": perf["ttft"]["mean"],
        "p50_ttft_confidence_half_width_ms": perf["ttft"]["half_width"],
        "p50_total_ms": perf["total"]["mean"],
        "p50_total_confidence_half_width_ms": perf["total"]["half_width"],
        "mean_prefill_tokens_per_second": perf["prefill"]["mean"],
        "prefill_tokens_per_second_confidence_half_width": perf["prefill"]["half_width"],
        "mean_decode_tokens_per_second": perf["decode"]["mean"],
        "decode_tokens_per_second_confidence_half_width": perf["decode"]["half_width"],
        "aggregate_generated_tokens_per_second": perf["decode"]["mean"],
        "aggregate_generated_tokens_per_second_confidence_half_width": perf["decode"]["half_width"],
        "peak_kv_bytes": perf["peak_kv_bytes"],
        "peak_device_bytes": perf["peak_device_bytes"],
        "prepared_artifact_bytes": perf.get("prepared_bytes", 0),
        "preparation_measured": True,
        "preparation_ms_mean": perf["prep"]["mean"] if dense else 0.0,
        "preparation_ms_stddev": perf["prep"]["stdev"] if dense else 0.0,
        "preparation_ms_confidence_half_width": perf["prep"]["half_width"] if dense else 0.0,
        "eviction_measured": not dense,
        "eviction_ms_mean": 0.0,
        "eviction_ms_stddev": 0.0,
        "eviction_ms_confidence_half_width": 0.0,
        "evidence_seed": 606,
        "evidence_id": "p6c:" + sid,
    }


def medium_perf(p6b: pathlib.Path, summary, name: str):
    item = summary["tactics"][name]
    sessions = item["session_rows"]
    ttft = stats(x["median_ttft_ms"] for x in sessions)
    total = stats(x["median_total_ms"] for x in sessions)
    prefill = stats(x["median_prefill_tokens_per_second"] for x in sessions)
    prep = stats(x["warmup_plan_preparation_ms"] for x in sessions)

    safe = name.replace("-", "_")
    decode_session = []
    for path in sorted(p6b.glob(f"session-*-{safe}-responses.json")):
        responses = load(path)
        vals = [
            float(r["metrics"]["decode_tokens_per_second"])
            for r in responses
            if float(r["metrics"]["decode_tokens_per_second"]) > 0.0
        ]
        if vals:
            decode_session.append(statistics.median(vals))
    if len(decode_session) != 3:
        raise RuntimeError(
            f"expected three decode sessions for {name}, got {len(decode_session)}"
        )
    decode = stats(decode_session)

    runtimes = [
        load(x) for x in sorted(p6b.glob(f"session-*-{safe}-runtime.json"))
    ]
    if len(runtimes) != 3:
        raise RuntimeError(f"expected three runtime snapshots for {name}")
    kv = {int(x["scheduler"]["cuda_kv_page_tokens"]) for x in runtimes}
    quantum = {int(x["scheduler"]["prefill_quantum_tokens"]) for x in runtimes}
    if len(kv) != 1 or quantum != {32}:
        raise RuntimeError(f"plan geometry changed for {name}: kv={kv} q={quantum}")

    return {
        "samples": 12,
        "ttft": ttft,
        "total": total,
        "prefill": prefill,
        "decode": decode,
        "prep": prep,
        "prepared_bytes": int(item["median_current_prepared_artifact_bytes"]),
        "peak_device_bytes": max(int(x["peak_device_bytes"]) for x in runtimes),
        "peak_kv_bytes": max(int(x["peak_kv_bytes"]) for x in runtimes),
        "kv_page_tokens": next(iter(kv)),
    }


def small_perf(reports, kv_page_tokens):
    if len(reports) != 3:
        raise RuntimeError("expected three small reuse8 reports")
    for d in reports:
        if d["schema"] != "air.benchmark.v11" or d["backend"] != "cuda":
            raise RuntimeError("invalid small reuse8 report")
        if d["prefill_block_linear_tactic"] != "batch-reuse8":
            raise RuntimeError("small reuse8 report selected wrong tactic")
    return {
        "samples": 12,
        "ttft": stats(x["summary"]["p50_ttft_ms"] for x in reports),
        "total": stats(x["summary"]["p50_total_ms"] for x in reports),
        "prefill": stats(x["summary"]["mean_prefill_tokens_per_second"] for x in reports),
        "decode": stats(x["summary"]["mean_decode_tokens_per_second"] for x in reports),
        "prep": stats([0.0]),
        "prepared_bytes": 0,
        "peak_device_bytes": max(int(x["summary"]["peak_device_bytes"]) for x in reports),
        "peak_kv_bytes": max(int(x["summary"]["peak_kv_bytes"]) for x in reports),
        "kv_page_tokens": kv_page_tokens,
    }


def validate_bootstrap_evictions(root: pathlib.Path, dense_bytes: int):
    values = []
    for path in sorted((root / "eviction").glob("eviction-*.json")):
        d = load(path)
        first, second = d["steps"]
        assert first["metrics"]["strategy_id"] == "dense-medium", first["metrics"]
        assert int(first["metrics"]["plan_preparation_bytes"]) == dense_bytes
        assert int(first["after"]["current_prepared_artifact_bytes"]) == dense_bytes
        assert second["metrics"]["strategy_id"] == "reuse8-small", second["metrics"]
        value = float(second["metrics"]["plan_eviction_ms"])
        assert value > 0.0
        assert int(second["after"]["current_prepared_artifact_bytes"]) == 0
        values.append(value)
    if len(values) != 5:
        raise RuntimeError("expected five eviction measurements")
    return values


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", type=pathlib.Path, required=True)
    ap.add_argument("--p6b", type=pathlib.Path, required=True)
    ap.add_argument("--out", type=pathlib.Path, required=True)
    ap.add_argument("--build", type=pathlib.Path, required=True)
    args = ap.parse_args()

    model = args.model
    p6b = args.p6b
    root = args.out
    build = args.build
    cli = build / "air-cli"
    verify = build / "air-verify"
    bench = build / "air-bench"
    probe = build / "air-strategy-probe"
    for tool in (cli, verify, bench, probe):
        if not tool.exists():
            raise RuntimeError(f"missing built executable: {tool}")

    print("[6C] fingerprint + prompt identity", flush=True)
    fingerprint = json.loads(capture([str(cli), "fingerprint", str(model)]))
    (root / "fingerprint.json").write_text(
        json.dumps(fingerprint, indent=2, sort_keys=True) + "\n"
    )

    prompts = root / "prompts"
    medium_path = prompts / "medium.txt"
    small_path = prompts / "small.txt"
    medium_text = (
        "AIR compares qualified physical implementations using the same semantic "
        "program, machine, scheduler, and deterministic output contract. "
    ) * 48
    small_text = (
        "AIR should release optional prepared GPU state when a small workload "
        "requires the lower-residency qualified execution plan."
    )
    medium_path.write_text(medium_text)
    small_path.write_text(small_text)
    medium_tokens = token_count(cli, model, medium_text)
    small_tokens = token_count(cli, model, small_text)

    p6b_summary = load(p6b / "prompt6b-prefill-tactic-summary.json")
    expected_medium = int(p6b_summary["tactics"]["reuse8"]["prompt_tokens"])
    if medium_tokens != expected_medium:
        raise RuntimeError(
            f"6B prompt identity mismatch: current={medium_tokens} evidence={expected_medium}"
        )
    if not (small_tokens <= 256 and 257 <= medium_tokens <= 2048):
        raise RuntimeError(
            f"workload classification mismatch small={small_tokens} medium={medium_tokens}"
        )
    (root / "prompt-token-counts.json").write_text(
        json.dumps(
            {"small_tokens": small_tokens, "medium_tokens": medium_tokens},
            indent=2,
        )
        + "\n"
    )

    print("[6C] strict 1.5B numerical qualification", flush=True)
    strict_dir = root / "strict"
    for width in (1, 8, 64):
        token_csv = ",".join(str(i) for i in range(1, width + 1))
        for name, tactic in (
            ("reuse8", "reuse8"),
            ("dense", "dense-f32-cublas"),
        ):
            print(f"[6C] verify {name} width={width}", flush=True)
            run(
                [
                    str(verify),
                    "-m",
                    str(model),
                    "--tokens",
                    token_csv,
                    "--generate",
                    "2",
                    "--top-k",
                    "8",
                    "--device",
                    "0",
                    "--atol",
                    "0.001",
                    "--cuda-prefill-block-linear",
                    tactic,
                    "--cuda-decode-block-linear",
                    "baseline",
                    "--cuda-decode-output-linear",
                    "baseline",
                    "--cuda-prefill-attention",
                    "baseline",
                    "--cuda-decode-attention",
                    "baseline",
                    "--output",
                    str(strict_dir / f"{name}-p{width}.json"),
                ],
                log=strict_dir / f"{name}-p{width}.txt",
            )

    print("[6C] measure small reuse8 destination lane", flush=True)
    small_dir = root / "small"
    for round_index in range(1, 4):
        run(
            [
                str(bench),
                "-m",
                str(model),
                "--backend",
                "cuda",
                "--device",
                "0",
                "--no-manifest",
                "--prompt-file",
                str(small_path),
                "--tokens",
                "2",
                "--warmup",
                "1",
                "--runs",
                "4",
                "--concurrency",
                "1",
                "--prefix-cache",
                "0",
                "--token-budget",
                "256",
                "--prefill-quantum",
                "32",
                "--cuda-prefill-block-linear",
                "reuse8",
                "--cuda-decode-block-linear",
                "baseline",
                "--cuda-decode-output-linear",
                "baseline",
                "--cuda-prefill-attention",
                "baseline",
                "--output",
                str(small_dir / f"reuse8-round-{round_index}.json"),
            ],
            log=small_dir / f"reuse8-round-{round_index}.txt",
        )

    print("[6C] build bootstrap schema-v10 manifest", flush=True)
    reuse = medium_perf(p6b, p6b_summary, "reuse8")
    dense = medium_perf(p6b, p6b_summary, "dense-f32-cublas")
    small_reports = [
        load(x) for x in sorted(small_dir.glob("reuse8-round-*.json"))
    ]
    small = small_perf(small_reports, reuse["kv_page_tokens"])
    manifest = {
        "schema_version": 10,
        "air_version": fingerprint["air_version"],
        "model_digest": fingerprint["model_digest"],
        "hardware_digest": fingerprint["hardware_digest"],
        "manifest_id": "air-p6c-1p5b-bootstrap",
        "strategies": [
            strategy(
                "reuse8-small",
                "small",
                small_tokens,
                "batch-reuse8",
                small,
                dense=False,
            ),
            strategy(
                "reuse8-medium",
                "medium",
                medium_tokens,
                "batch-reuse8",
                reuse,
                dense=False,
            ),
            strategy(
                "dense-medium",
                "medium",
                medium_tokens,
                "dense-f32-cublas",
                dense,
                dense=True,
            ),
        ],
    }
    bootstrap = root / "bootstrap-manifest.json"
    bootstrap.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    (root / "bootstrap-inputs.json").write_text(
        json.dumps(
            {
                "medium_tokens": medium_tokens,
                "small_tokens": small_tokens,
                "medium_reuse": reuse,
                "medium_dense": dense,
                "small_reuse": small,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n"
    )
    dense_bytes = int(dense["prepared_bytes"])
    print(
        f"[6C] dense prepared={dense_bytes / 1024**3:.3f} GiB "
        f"prep={dense['prep']['mean']:.3f} ms",
        flush=True,
    )

    print("[6C] qualification-only dense -> reuse8 eviction measurements", flush=True)
    for round_index in range(1, 6):
        print(f"[6C] eviction round {round_index}/5", flush=True)
        run_probe(
            probe,
            model,
            bootstrap,
            medium_path,
            small_path,
            "eviction",
            root / "eviction" / f"eviction-{round_index}.json",
            root / "eviction" / f"eviction-{round_index}.txt",
            root / "telemetry" / f"eviction-{round_index}.json",
            allow_unknown_eviction=True,
        )

    eviction_values = validate_bootstrap_evictions(root, dense_bytes)
    ev = stats(eviction_values)
    print(
        f"[6C] measured dense eviction mean={ev['mean']:.3f} ms "
        f"95% half-width={ev['half_width']:.3f} ms",
        flush=True,
    )

    for item in manifest["strategies"]:
        if item["strategy_id"] == "dense-medium":
            item["eviction_measured"] = True
            item["eviction_ms_mean"] = ev["mean"]
            item["eviction_ms_stddev"] = ev["stdev"]
            item["eviction_ms_confidence_half_width"] = ev["half_width"]
    manifest["manifest_id"] = "air-p6c-1p5b-final"
    final_manifest = root / "final-manifest.json"
    final_manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")

    print("[6C] product path: hot dense", flush=True)
    run_probe(
        probe,
        model,
        final_manifest,
        medium_path,
        small_path,
        "hot-dense",
        root / "product" / "hot-dense.json",
        root / "product" / "hot-dense.txt",
        root / "telemetry" / "hot-dense.json",
    )

    print("[6C] product path: measured eviction", flush=True)
    run_probe(
        probe,
        model,
        final_manifest,
        medium_path,
        small_path,
        "eviction",
        root / "product" / "final-eviction.json",
        root / "product" / "final-eviction.txt",
        root / "telemetry" / "final-eviction.json",
    )

    print("[6C] product path: three transition cycles", flush=True)
    run_probe(
        probe,
        model,
        final_manifest,
        medium_path,
        small_path,
        "oscillation",
        root / "product" / "oscillation.json",
        root / "product" / "oscillation.txt",
        root / "telemetry" / "oscillation.json",
        cycles=3,
    )

    low_budget = dense_bytes - 1
    print("[6C] product path: explicit prepared-memory rejection", flush=True)
    run(
        [
            str(bench),
            "-m",
            str(model),
            "--backend",
            "auto",
            "--device",
            "0",
            "--manifest",
            str(final_manifest),
            "--require-manifest",
            "--prompt-file",
            str(medium_path),
            "--tokens",
            "2",
            "--warmup",
            "0",
            "--runs",
            "1",
            "--concurrency",
            "1",
            "--prefix-cache",
            "0",
            "--token-budget",
            "256",
            "--prefill-quantum",
            "32",
            "--strategy-objective",
            "maximum-throughput",
            "--strategy-horizon-tokens",
            "0",
            "--strategy-prepared-memory-budget-bytes",
            str(low_budget),
            "--output",
            str(root / "product" / "budget-rejection.json"),
        ],
        log=root / "product" / "budget-rejection.txt",
    )

    print("[6C] product path: minimum-vram objective", flush=True)
    run(
        [
            str(bench),
            "-m",
            str(model),
            "--backend",
            "auto",
            "--device",
            "0",
            "--manifest",
            str(final_manifest),
            "--require-manifest",
            "--prompt-file",
            str(medium_path),
            "--tokens",
            "2",
            "--warmup",
            "0",
            "--runs",
            "1",
            "--concurrency",
            "1",
            "--prefix-cache",
            "0",
            "--token-budget",
            "256",
            "--prefill-quantum",
            "32",
            "--strategy-objective",
            "minimum-vram",
            "--strategy-horizon-tokens",
            "0",
            "--output",
            str(root / "product" / "minimum-vram.json"),
        ],
        log=root / "product" / "minimum-vram.txt",
    )

    print("[6C] validate product transition behavior", flush=True)
    hot = load(root / "product" / "hot-dense.json")
    if [x["metrics"]["strategy_id"] for x in hot["steps"]] != [
        "dense-medium",
        "dense-medium",
    ]:
        raise RuntimeError("hot-dense scenario selected wrong strategies")
    if int(hot["steps"][0]["after"]["current_prepared_artifact_bytes"]) != dense_bytes:
        raise RuntimeError("dense artifact not resident after first hot-dense request")
    if float(hot["steps"][1]["metrics"]["plan_preparation_ms"]) != 0.0:
        raise RuntimeError("hot dense request unexpectedly re-prepared artifact")
    if float(hot["steps"][1]["metrics"]["plan_eviction_ms"]) != 0.0:
        raise RuntimeError("hot dense request unexpectedly evicted artifact")
    if int(hot["steps"][1]["after"]["current_prepared_artifact_bytes"]) != dense_bytes:
        raise RuntimeError("hot dense artifact was not retained")

    final_ev = load(root / "product" / "final-eviction.json")
    if [x["metrics"]["strategy_id"] for x in final_ev["steps"]] != [
        "dense-medium",
        "reuse8-small",
    ]:
        raise RuntimeError("final eviction scenario selected wrong strategies")
    if float(final_ev["steps"][1]["metrics"]["plan_eviction_ms"]) <= 0.0:
        raise RuntimeError("final product eviction was not measured")
    if int(final_ev["steps"][1]["after"]["current_prepared_artifact_bytes"]) != 0:
        raise RuntimeError("hidden dense residency remained after final eviction")

    oscillation = load(root / "product" / "oscillation.json")
    steps = oscillation["steps"]
    if len(steps) != 6:
        raise RuntimeError("oscillation did not produce six transition steps")
    reprep = []
    osc_evict = []
    for index, step in enumerate(steps):
        if index % 2 == 0:
            if step["metrics"]["strategy_id"] != "dense-medium":
                raise RuntimeError("oscillation medium step did not select dense")
            if int(step["after"]["current_prepared_artifact_bytes"]) != dense_bytes:
                raise RuntimeError("dense artifact absent after oscillation medium step")
            prep_ms = float(step["metrics"]["plan_preparation_ms"])
            if index > 0:
                if prep_ms <= 0.0:
                    raise RuntimeError("dense was not re-prepared after eviction")
                reprep.append(prep_ms)
        else:
            if step["metrics"]["strategy_id"] != "reuse8-small":
                raise RuntimeError("oscillation small step did not select reuse8")
            evict_ms = float(step["metrics"]["plan_eviction_ms"])
            if evict_ms <= 0.0:
                raise RuntimeError("oscillation did not evict dense state")
            osc_evict.append(evict_ms)
            if int(step["after"]["current_prepared_artifact_bytes"]) != 0:
                raise RuntimeError("hidden dense residency remained in oscillation")
    if int(oscillation["snapshot"]["current_prepared_artifact_bytes"]) != 0:
        raise RuntimeError("oscillation final snapshot retained dense state")

    budget = load(root / "product" / "budget-rejection.json")
    budget_run = budget["runs"][0]
    if budget_run["strategy_id"] != "reuse8-medium":
        raise RuntimeError("low prepared-memory budget did not select reuse8")
    dense_trace = next(
        x
        for x in budget_run["strategy_candidates"]
        if x["strategy_id"] == "dense-medium"
    )
    if dense_trace["memory_feasible"] is not False:
        raise RuntimeError("dense was not marked memory-infeasible under budget")
    if dense_trace["disposition"] != "rejected:memory-infeasible":
        raise RuntimeError("dense budget rejection reason changed")
    if int(budget["summary"]["prepared_artifact_bytes"]) != 0:
        raise RuntimeError("budget-rejection run retained dense artifact")

    minimum = load(root / "product" / "minimum-vram.json")
    minimum_run = minimum["runs"][0]
    if minimum_run["strategy_id"] != "reuse8-medium":
        raise RuntimeError("minimum-vram objective did not select reuse8")
    if int(minimum["summary"]["prepared_artifact_bytes"]) != 0:
        raise RuntimeError("minimum-vram run retained dense artifact")

    telemetry = []
    for path in sorted((root / "telemetry").glob("*.json")):
        telemetry.extend(
            x for x in load(path) if "free_mib" in x and "used_mib" in x
        )
    if not telemetry:
        raise RuntimeError("no GPU telemetry retained")
    min_free = min(float(x["free_mib"]) for x in telemetry)
    max_used = max(float(x["used_mib"]) for x in telemetry)
    max_temp = max(float(x["temp_c"]) for x in telemetry)

    reuse_rate = float(
        p6b_summary["tactics"]["reuse8"]["median_prefill_tokens_per_second"]
    )
    dense_rate = float(
        p6b_summary["tactics"]["dense-f32-cublas"][
            "median_prefill_tokens_per_second"
        ]
    )
    prep_ms = float(dense["prep"]["mean"])
    saved_ms = 1000.0 / reuse_rate - 1000.0 / dense_rate
    prep_break_even = prep_ms / saved_ms
    roundtrip_break_even = (prep_ms + ev["mean"]) / saved_ms

    final = {
        "schema": "air.prompt6c.transition-economics.v1",
        "dense_prepared_artifact_bytes": dense_bytes,
        "dense_prepared_artifact_gib": dense_bytes / (1024**3),
        "dense_preparation_ms_mean": prep_ms,
        "dense_eviction_ms_mean": ev["mean"],
        "dense_eviction_ms_confidence_half_width": ev["half_width"],
        "dense_eviction_samples_ms": eviction_values,
        "oscillation_repreparation_ms": reprep,
        "oscillation_eviction_ms": osc_evict,
        "minimum_observed_free_vram_mib": min_free,
        "maximum_observed_used_vram_mib": max_used,
        "maximum_observed_temperature_c": max_temp,
        "reuse8_prefill_tokens_per_second": reuse_rate,
        "dense_prefill_tokens_per_second": dense_rate,
        "saved_ms_per_prefill_token": saved_ms,
        "preparation_only_break_even_prefill_tokens": prep_break_even,
        "roundtrip_prep_plus_eviction_break_even_prefill_tokens": roundtrip_break_even,
        "budget_rejection_strategy": budget_run["strategy_id"],
        "minimum_vram_strategy": minimum_run["strategy_id"],
        "hot_dense_second_preparation_ms": float(
            hot["steps"][1]["metrics"]["plan_preparation_ms"]
        ),
        "final_product_eviction_ms": float(
            final_ev["steps"][1]["metrics"]["plan_eviction_ms"]
        ),
        "hidden_dense_residency_after_eviction": False,
    }
    (root / "prompt6c-transition-summary.json").write_text(
        json.dumps(final, indent=2, sort_keys=True) + "\n"
    )

    print("PROMPT6C_TRANSITION_ECONOMICS=PASS")
    print(f"dense_prepared_artifact_gib={final['dense_prepared_artifact_gib']:.3f}")
    print(f"dense_preparation_ms_mean={prep_ms:.6f}")
    print(f"dense_eviction_ms_mean={ev['mean']:.6f}")
    print(
        "dense_repreparation_ms_samples="
        + ",".join(f"{x:.6f}" for x in reprep)
    )
    print(f"minimum_observed_free_vram_mib={min_free:.1f}")
    print(f"maximum_observed_used_vram_mib={max_used:.1f}")
    print(f"maximum_observed_temperature_c={max_temp:.1f}")
    print(f"preparation_only_break_even_prefill_tokens={prep_break_even:.3f}")
    print(f"roundtrip_break_even_prefill_tokens={roundtrip_break_even:.3f}")
    print("budget_rejection_strategy=" + budget_run["strategy_id"])
    print("minimum_vram_strategy=" + minimum_run["strategy_id"])
    print("hidden_dense_residency_after_eviction=NO")


if __name__ == "__main__":
    main()
