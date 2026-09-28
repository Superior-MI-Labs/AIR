#!/usr/bin/env python3
import argparse
import json
import math
import pathlib
import sys

def load(path):
    return json.loads(pathlib.Path(path).read_text())

def summary(doc):
    decisions = doc.get("decisions", [])
    max_abs = [float(d.get("logits", {}).get("max_abs_error", 0.0)) for d in decisions]
    rms = [float(d.get("logits", {}).get("rms_error", 0.0)) for d in decisions]
    mean_abs = [float(d.get("logits", {}).get("mean_abs_error", 0.0)) for d in decisions]
    return {
        "schema": doc.get("schema"),
        "decisions": len(decisions),
        "finite": bool(doc.get("summary", {}).get("finite")),
        "top1_parity": bool(doc.get("summary", {}).get("top1_parity")),
        "max_abs_error": max(max_abs, default=0.0),
        "max_rms_error": max(rms, default=0.0),
        "max_mean_abs_error": max(mean_abs, default=0.0),
        "teacher_forced_tokens": doc.get("teacher_forced_tokens", []),
        "decision_tops": [
            (
                d.get("reference_top"),
                d.get("cuda_top"),
                bool(d.get("top1_match", False)),
            )
            for d in decisions
        ],
    }

def not_worse(current, baseline, rel=0.01, abs_eps=1e-7):
    limit = baseline * (1.0 + rel) + abs_eps
    return current <= limit, limit

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--current", required=True)
    ap.add_argument("--baseline", required=True)
    ap.add_argument("--output")
    args = ap.parse_args()

    current_doc = load(args.current)
    baseline_doc = load(args.baseline)
    current = summary(current_doc)
    baseline = summary(baseline_doc)

    checks = {}
    checks["schema_match"] = current["schema"] == baseline["schema"]
    checks["decision_count_match"] = current["decisions"] == baseline["decisions"] and current["decisions"] > 0
    checks["current_finite"] = current["finite"]
    checks["baseline_finite"] = baseline["finite"]
    checks["current_top1_parity"] = current["top1_parity"]
    checks["baseline_top1_parity"] = baseline["top1_parity"]
    checks["teacher_forced_token_history_match"] = (
        current["teacher_forced_tokens"] == baseline["teacher_forced_tokens"]
    )
    checks["decision_top_tokens_match"] = (
        current["decision_tops"] == baseline["decision_tops"]
    )

    max_ok, max_limit = not_worse(current["max_abs_error"], baseline["max_abs_error"])
    rms_ok, rms_limit = not_worse(current["max_rms_error"], baseline["max_rms_error"])
    mean_ok, mean_limit = not_worse(current["max_mean_abs_error"], baseline["max_mean_abs_error"])
    checks["max_abs_error_not_worse"] = max_ok
    checks["max_rms_error_not_worse"] = rms_ok
    checks["max_mean_abs_error_not_worse"] = mean_ok

    result = {
        "schema": "air.verification.baseline-comparison.v1",
        "current": current,
        "baseline": baseline,
        "limits": {
            "relative_headroom": 0.01,
            "absolute_epsilon": 1e-7,
            "max_abs_error": max_limit,
            "max_rms_error": rms_limit,
            "max_mean_abs_error": mean_limit,
        },
        "checks": checks,
        "pass": all(checks.values()),
    }

    text = json.dumps(result, indent=2, sort_keys=True)
    print(text)
    if args.output:
        pathlib.Path(args.output).write_text(text + "\n")
    return 0 if result["pass"] else 1

if __name__ == "__main__":
    raise SystemExit(main())
