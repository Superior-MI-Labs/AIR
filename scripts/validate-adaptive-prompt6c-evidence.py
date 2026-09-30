#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import statistics


def load(path: pathlib.Path):
    return json.loads(path.read_text())


def require(condition: bool, message: str):
    if not condition:
        raise RuntimeError(message)


def main():
    ap = argparse.ArgumentParser(
        description="Validate retained AIR Prompt 6C exact-plan evidence without rerunning benchmarks"
    )
    ap.add_argument("evidence_dir", type=pathlib.Path)
    args = ap.parse_args()
    root = args.evidence_dir

    required = [
        root / "bootstrap-inputs.json",
        root / "final-manifest.json",
        root / "product" / "hot-dense.json",
        root / "product" / "final-eviction.json",
        root / "product" / "oscillation.json",
        root / "product" / "budget-rejection.json",
        root / "product" / "minimum-vram.json",
    ]
    for path in required:
        require(path.is_file(), f"missing retained evidence: {path}")

    inputs = load(root / "bootstrap-inputs.json")
    manifest = load(root / "final-manifest.json")

    dense_strategy = next(
        (x for x in manifest["strategies"] if x["strategy_id"] == "dense-medium"),
        None,
    )
    require(dense_strategy is not None, "final manifest missing dense-medium")
    dense_bytes = int(dense_strategy["prepared_artifact_bytes"])
    require(dense_bytes > 0, "dense-medium has no prepared artifact")

    dense_perf = inputs["medium_dense"]
    reuse_perf = inputs["medium_reuse"]
    dense_prep_ms = float(dense_perf["prep"]["mean"])
    reuse_rate = float(reuse_perf["prefill"]["mean"])
    dense_rate = float(dense_perf["prefill"]["mean"])
    require(dense_prep_ms > 0.0, "dense cold preparation mean is not positive")
    require(dense_rate > reuse_rate > 0.0, "dense/reuse8 product rates are invalid")

    hot = load(root / "product" / "hot-dense.json")
    hot_steps = hot["steps"]
    require(len(hot_steps) == 2, "hot-dense scenario did not produce two steps")
    require(
        [x["metrics"]["strategy_id"] for x in hot_steps]
        == ["dense-medium", "dense-medium"],
        "hot-dense selected unexpected strategies",
    )
    require(
        int(hot_steps[0]["after"]["current_prepared_artifact_bytes"]) == dense_bytes,
        "dense artifact not resident after first hot-dense request",
    )

    second = hot_steps[1]
    require(
        bool(second["metrics"]["strategy_prepared_state_hot"]),
        "second dense request was not classified prepared-state hot",
    )
    require(
        int(second["before"]["current_prepared_artifact_bytes"]) == dense_bytes,
        "dense artifact missing before second hot request",
    )
    require(
        int(second["metrics"]["plan_preparation_bytes"]) == 0,
        "second hot dense request forecast new preparation bytes",
    )
    require(
        int(second["after"]["current_prepared_artifact_bytes"]) == dense_bytes,
        "dense artifact not retained after second hot request",
    )
    hot_prepare_ms = float(second["metrics"]["plan_preparation_ms"])
    hot_trim_ms = float(second["metrics"]["plan_eviction_ms"])
    hot_check_ms = hot_prepare_ms + hot_trim_ms
    require(
        hot_check_ms < dense_prep_ms,
        "hot idempotent trim/prepare checks were not cheaper than cold dense preparation",
    )

    final_ev = load(root / "product" / "final-eviction.json")
    ev_steps = final_ev["steps"]
    require(
        [x["metrics"]["strategy_id"] for x in ev_steps]
        == ["dense-medium", "reuse8-small"],
        "final eviction scenario selected unexpected strategies",
    )
    require(
        int(ev_steps[0]["after"]["current_prepared_artifact_bytes"]) == dense_bytes,
        "dense artifact absent before final eviction",
    )
    require(
        float(ev_steps[1]["metrics"]["plan_eviction_ms"]) > 0.0,
        "final product eviction did not record positive elapsed time",
    )
    require(
        int(ev_steps[1]["after"]["current_prepared_artifact_bytes"]) == 0,
        "dense artifact remained resident after final eviction",
    )

    eviction_samples = []
    for path in sorted((root / "eviction").glob("eviction-*.json")):
        data = load(path)
        first, small = data["steps"]
        require(
            first["metrics"]["strategy_id"] == "dense-medium",
            f"{path.name}: first step did not select dense-medium",
        )
        require(
            int(first["after"]["current_prepared_artifact_bytes"]) == dense_bytes,
            f"{path.name}: dense artifact was not resident",
        )
        require(
            small["metrics"]["strategy_id"] == "reuse8-small",
            f"{path.name}: second step did not select reuse8-small",
        )
        value = float(small["metrics"]["plan_eviction_ms"])
        require(value > 0.0, f"{path.name}: eviction elapsed time not positive")
        require(
            int(small["after"]["current_prepared_artifact_bytes"]) == 0,
            f"{path.name}: hidden dense residency after eviction",
        )
        eviction_samples.append(value)
    require(len(eviction_samples) == 5, "expected five qualification eviction samples")

    oscillation = load(root / "product" / "oscillation.json")
    steps = oscillation["steps"]
    require(len(steps) == 6, "oscillation did not produce six steps")
    reprep_ms = []
    osc_evict_ms = []
    for index, step in enumerate(steps):
        if index % 2 == 0:
            require(
                step["metrics"]["strategy_id"] == "dense-medium",
                f"oscillation step {index}: expected dense-medium",
            )
            require(
                int(step["after"]["current_prepared_artifact_bytes"]) == dense_bytes,
                f"oscillation step {index}: dense artifact not resident",
            )
            if index > 0:
                require(
                    int(step["before"]["current_prepared_artifact_bytes"]) == 0,
                    f"oscillation step {index}: dense unexpectedly resident before reprepare",
                )
                require(
                    int(step["metrics"]["plan_preparation_bytes"]) == dense_bytes,
                    f"oscillation step {index}: dense reprepare byte count mismatch",
                )
                prep_ms = float(step["metrics"]["plan_preparation_ms"])
                require(
                    prep_ms > 0.0,
                    f"oscillation step {index}: dense reprepare time not positive",
                )
                reprep_ms.append(prep_ms)
        else:
            require(
                step["metrics"]["strategy_id"] == "reuse8-small",
                f"oscillation step {index}: expected reuse8-small",
            )
            require(
                int(step["before"]["current_prepared_artifact_bytes"]) == dense_bytes,
                f"oscillation step {index}: dense not resident before eviction",
            )
            evict_ms = float(step["metrics"]["plan_eviction_ms"])
            require(
                evict_ms > 0.0,
                f"oscillation step {index}: eviction elapsed time not positive",
            )
            require(
                int(step["after"]["current_prepared_artifact_bytes"]) == 0,
                f"oscillation step {index}: dense remained resident after eviction",
            )
            osc_evict_ms.append(evict_ms)
    require(
        int(oscillation["snapshot"]["current_prepared_artifact_bytes"]) == 0,
        "oscillation final snapshot retained dense artifact",
    )
    require(len(reprep_ms) == 2, "expected two measured dense repreparations")
    require(len(osc_evict_ms) == 3, "expected three measured oscillation evictions")

    budget = load(root / "product" / "budget-rejection.json")
    budget_run = budget["runs"][0]
    require(
        budget_run["strategy_id"] == "reuse8-medium",
        "prepared-memory budget did not select reuse8-medium",
    )
    dense_trace = next(
        x
        for x in budget_run["strategy_candidates"]
        if x["strategy_id"] == "dense-medium"
    )
    require(
        dense_trace["memory_feasible"] is False,
        "dense was not marked memory-infeasible under prepared-memory budget",
    )
    require(
        dense_trace["disposition"] == "rejected:memory-infeasible",
        "dense prepared-memory rejection disposition changed",
    )
    require(
        int(budget["summary"]["prepared_artifact_bytes"]) == 0,
        "budget-rejection run retained dense artifact",
    )

    minimum = load(root / "product" / "minimum-vram.json")
    minimum_run = minimum["runs"][0]
    require(
        minimum_run["strategy_id"] == "reuse8-medium",
        "minimum-vram objective did not select reuse8-medium",
    )
    require(
        int(minimum["summary"]["prepared_artifact_bytes"]) == 0,
        "minimum-vram run retained dense artifact",
    )

    telemetry = []
    for path in sorted((root / "telemetry").glob("*.json")):
        telemetry.extend(
            x for x in load(path) if "free_mib" in x and "used_mib" in x
        )
    require(telemetry, "no retained GPU telemetry found")

    min_free = min(float(x["free_mib"]) for x in telemetry)
    max_used = max(float(x["used_mib"]) for x in telemetry)
    max_temp = max(float(x["temp_c"]) for x in telemetry)

    eviction_mean = statistics.fmean(eviction_samples)
    saved_ms_per_token = 1000.0 / reuse_rate - 1000.0 / dense_rate
    require(saved_ms_per_token > 0.0, "dense product plan does not save prefill time")
    prep_break_even = dense_prep_ms / saved_ms_per_token
    roundtrip_break_even = (dense_prep_ms + eviction_mean) / saved_ms_per_token

    summary = {
        "schema": "air.prompt6c.transition-economics.v1",
        "validation_mode": "retained-evidence",
        "dense_prepared_artifact_bytes": dense_bytes,
        "dense_prepared_artifact_gib": dense_bytes / (1024**3),
        "reuse8_prefill_tokens_per_second": reuse_rate,
        "dense_prefill_tokens_per_second": dense_rate,
        "dense_vs_reuse8_prefill_ratio": dense_rate / reuse_rate,
        "dense_preparation_ms_mean": dense_prep_ms,
        "dense_eviction_ms_mean": eviction_mean,
        "dense_eviction_samples_ms": eviction_samples,
        "oscillation_repreparation_ms": reprep_ms,
        "oscillation_eviction_ms": osc_evict_ms,
        "hot_dense_second_preparation_bytes": int(
            second["metrics"]["plan_preparation_bytes"]
        ),
        "hot_dense_second_preparation_ms": hot_prepare_ms,
        "hot_dense_second_eviction_check_ms": hot_trim_ms,
        "hot_dense_second_prepared_state_hot": bool(
            second["metrics"]["strategy_prepared_state_hot"]
        ),
        "minimum_observed_free_vram_mib": min_free,
        "maximum_observed_used_vram_mib": max_used,
        "maximum_observed_temperature_c": max_temp,
        "saved_ms_per_prefill_token": saved_ms_per_token,
        "preparation_only_break_even_prefill_tokens": prep_break_even,
        "roundtrip_prep_plus_eviction_break_even_prefill_tokens": roundtrip_break_even,
        "budget_rejection_strategy": budget_run["strategy_id"],
        "minimum_vram_strategy": minimum_run["strategy_id"],
        "hidden_dense_residency_after_eviction": False,
    }
    out = root / "prompt6c-transition-summary-retained.json"
    out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")

    print("PROMPT6C_RETAINED_EVIDENCE_VALIDATION=PASS")
    print(f"dense_vs_reuse8_prefill_ratio={summary['dense_vs_reuse8_prefill_ratio']:.3f}")
    print(f"dense_prepared_artifact_gib={summary['dense_prepared_artifact_gib']:.3f}")
    print(f"dense_preparation_ms_mean={dense_prep_ms:.6f}")
    print(f"dense_eviction_ms_mean={eviction_mean:.6f}")
    print(f"hot_dense_second_preparation_bytes={summary['hot_dense_second_preparation_bytes']}")
    print(f"hot_dense_second_preparation_ms={hot_prepare_ms:.6f}")
    print(f"hot_dense_second_eviction_check_ms={hot_trim_ms:.6f}")
    print(f"hot_dense_second_prepared_state_hot={str(summary['hot_dense_second_prepared_state_hot']).upper()}")
    print(f"dense_repreparation_ms_samples={','.join(f'{x:.6f}' for x in reprep_ms)}")
    print(f"minimum_observed_free_vram_mib={min_free:.1f}")
    print(f"maximum_observed_used_vram_mib={max_used:.1f}")
    print(f"maximum_observed_temperature_c={max_temp:.1f}")
    print(f"preparation_only_break_even_prefill_tokens={prep_break_even:.3f}")
    print(f"roundtrip_break_even_prefill_tokens={roundtrip_break_even:.3f}")
    print(f"budget_rejection_strategy={budget_run['strategy_id']}")
    print(f"minimum_vram_strategy={minimum_run['strategy_id']}")
    print("hidden_dense_residency_after_eviction=NO")
    print(f"summary={out}")


if __name__ == "__main__":
    main()
