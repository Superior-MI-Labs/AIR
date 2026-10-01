#!/usr/bin/env python3
"""Prompt 7G source-clean ComfyUI transition/residency observer.

The serve mode launches the pinned ComfyUI checkout without modifying it and
wraps only existing model-management boundaries to record diagnostic evidence.
The analyze mode summarizes those events after the unchanged Prompt 7B oracle
has executed.

Important measurement boundary:
- recorded call durations are host-call durations;
- runtime-reported loaded bytes are ComfyUI state at observed boundaries;
- external GPU telemetry is sampled device state;
- asynchronous transfer completion and per-layer DynamicVRAM residency are not
  inferred from these measurements.
"""

from __future__ import annotations

import argparse
import functools
import json
import os
import pathlib
import runpy
import statistics
import sys
import threading
import time
from typing import Any

EXPECTED_PIXEL_SHA256 = "c3a4278c608408df5019cf15162707e29263dee76e7a0114a6b1dcf2c29e1aa6"
REQUIRED_COMPONENTS = ("text-encoder", "flux2-denoiser", "vae")


def primitive(value: Any) -> Any:
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, (list, tuple)):
        return [primitive(x) for x in value]
    if isinstance(value, dict):
        return {str(k): primitive(v) for k, v in value.items()}
    return str(value)


def component_label(class_name: str) -> str:
    lower = class_name.lower()
    if "flux2temodel" in lower or ("flux2" in lower and "te" in lower):
        return "text-encoder"
    if "autoencoder" in lower or "vae" in lower:
        return "vae"
    if "flux2" in lower:
        return "flux2-denoiser"
    return "unknown"


class EventWriter:
    def __init__(self, path: pathlib.Path):
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.lock = threading.Lock()
        self.sequence = 0

    def write(self, event: dict[str, Any]) -> None:
        with self.lock:
            self.sequence += 1
            record = {
                "schema": "air.prompt7g.transition-event.v1",
                "sequence": self.sequence,
                "wall_time_ns": time.time_ns(),
                "monotonic_ns": time.monotonic_ns(),
                **primitive(event),
            }
            with self.path.open("a", encoding="utf-8") as fh:
                fh.write(json.dumps(record, sort_keys=True) + "\n")
                fh.flush()


def safe_call(fn, default=None):
    try:
        return fn()
    except Exception:
        return default


def patcher_snapshot(patcher: Any) -> dict[str, Any]:
    if patcher is None:
        return {}
    model_obj = getattr(patcher, "model", None)
    class_name = type(model_obj).__name__ if model_obj is not None else type(patcher).__name__
    return {
        "patcher_class": type(patcher).__name__,
        "model_class": class_name,
        "component_label": component_label(class_name),
        "python_identity": hex(id(patcher)),
        "model_size_bytes": safe_call(lambda: int(patcher.model_size())),
        "loaded_size_bytes": safe_call(lambda: int(patcher.loaded_size())),
        "loaded_ram_size_bytes": safe_call(lambda: int(patcher.loaded_ram_size())),
        "current_loaded_device": safe_call(lambda: str(patcher.current_loaded_device())),
        "load_device": str(getattr(patcher, "load_device", "")),
        "offload_device": str(getattr(patcher, "offload_device", "")),
        "dynamic": safe_call(lambda: bool(patcher.is_dynamic())),
        "model_loaded_weight_memory_bytes": primitive(
            getattr(model_obj, "model_loaded_weight_memory", None)
        ),
        "model_offload_buffer_memory_bytes": primitive(
            getattr(model_obj, "model_offload_buffer_memory", None)
        ),
    }


def loaded_snapshot(loaded: Any) -> dict[str, Any]:
    patcher = safe_call(lambda: loaded.model)
    out = patcher_snapshot(patcher)
    out.update(
        {
            "loaded_model_python_identity": hex(id(loaded)),
            "loaded_model_device": str(getattr(loaded, "device", "")),
            "currently_used": primitive(getattr(loaded, "currently_used", None)),
            "model_memory_bytes": safe_call(lambda: int(loaded.model_memory())),
            "model_loaded_memory_bytes": safe_call(lambda: int(loaded.model_loaded_memory())),
            "model_offloaded_memory_bytes": safe_call(lambda: int(loaded.model_offloaded_memory())),
        }
    )
    return out


def all_loaded_snapshot(mm: Any) -> list[dict[str, Any]]:
    return [loaded_snapshot(x) for x in list(getattr(mm, "current_loaded_models", []))]


def device_free_bytes(mm: Any, device: Any) -> int | None:
    if device is None:
        return None
    if safe_call(lambda: mm.is_device_cpu(device), False):
        return None
    return safe_call(lambda: int(mm.get_free_memory(device)))


def install_probe(mm: Any, model_patcher_module: Any, writer: EventWriter) -> None:
    def wrap_loaded_method(name: str) -> None:
        original = getattr(mm.LoadedModel, name)

        @functools.wraps(original)
        def wrapped(self, *args, **kwargs):
            before = loaded_snapshot(self)
            free_before = device_free_bytes(mm, getattr(self, "device", None))
            start = time.monotonic_ns()
            writer.write(
                {
                    "event": "call",
                    "phase": "begin",
                    "call": f"LoadedModel.{name}",
                    "subject": before,
                    "device_free_bytes": free_before,
                    "loaded_models": all_loaded_snapshot(mm),
                }
            )
            status = "ok"
            result = None
            error = None
            try:
                result = original(self, *args, **kwargs)
                return result
            except Exception as exc:
                status = "error"
                error = repr(exc)
                raise
            finally:
                end = time.monotonic_ns()
                writer.write(
                    {
                        "event": "call",
                        "phase": "end",
                        "call": f"LoadedModel.{name}",
                        "status": status,
                        "error": error,
                        "duration_ms": (end - start) / 1_000_000.0,
                        "result": primitive(result),
                        "subject_before": before,
                        "subject_after": loaded_snapshot(self),
                        "device_free_bytes_before": free_before,
                        "device_free_bytes_after": device_free_bytes(
                            mm, getattr(self, "device", None)
                        ),
                        "loaded_models_after": all_loaded_snapshot(mm),
                    }
                )

        setattr(mm.LoadedModel, name, wrapped)

    def wrap_patcher_method(name: str) -> None:
        original = getattr(model_patcher_module.ModelPatcher, name)

        @functools.wraps(original)
        def wrapped(self, *args, **kwargs):
            before = patcher_snapshot(self)
            start = time.monotonic_ns()
            writer.write(
                {
                    "event": "call",
                    "phase": "begin",
                    "call": f"ModelPatcher.{name}",
                    "subject": before,
                }
            )
            status = "ok"
            result = None
            error = None
            try:
                result = original(self, *args, **kwargs)
                return result
            except Exception as exc:
                status = "error"
                error = repr(exc)
                raise
            finally:
                end = time.monotonic_ns()
                writer.write(
                    {
                        "event": "call",
                        "phase": "end",
                        "call": f"ModelPatcher.{name}",
                        "status": status,
                        "error": error,
                        "duration_ms": (end - start) / 1_000_000.0,
                        "result": primitive(result),
                        "subject_before": before,
                        "subject_after": patcher_snapshot(self),
                        "loaded_models_after": all_loaded_snapshot(mm),
                    }
                )

        setattr(model_patcher_module.ModelPatcher, name, wrapped)

    def wrap_global(name: str) -> None:
        original = getattr(mm, name)

        @functools.wraps(original)
        def wrapped(*args, **kwargs):
            before = all_loaded_snapshot(mm)
            start = time.monotonic_ns()
            writer.write(
                {
                    "event": "call",
                    "phase": "begin",
                    "call": f"model_management.{name}",
                    "loaded_models": before,
                }
            )
            status = "ok"
            result = None
            error = None
            try:
                result = original(*args, **kwargs)
                return result
            except Exception as exc:
                status = "error"
                error = repr(exc)
                raise
            finally:
                end = time.monotonic_ns()
                writer.write(
                    {
                        "event": "call",
                        "phase": "end",
                        "call": f"model_management.{name}",
                        "status": status,
                        "error": error,
                        "duration_ms": (end - start) / 1_000_000.0,
                        "result": primitive(result),
                        "loaded_models_before": before,
                        "loaded_models_after": all_loaded_snapshot(mm),
                    }
                )

        setattr(mm, name, wrapped)

    for method in ("model_load", "model_unload", "model_use_more_vram"):
        wrap_loaded_method(method)

    for method in ("partially_load", "partially_unload"):
        wrap_patcher_method(method)

    for function in ("load_models_gpu", "free_memory", "unload_all_models"):
        wrap_global(function)

    cli_module = sys.modules.get("comfy.cli_args")
    cli_args = getattr(cli_module, "args", None)
    writer.write(
        {
            "event": "probe-installed",
            "cli_contract": {
                "deterministic": primitive(getattr(cli_args, "deterministic", None)),
                "cache_none": primitive(getattr(cli_args, "cache_none", None)),
                "disable_all_custom_nodes": primitive(
                    getattr(cli_args, "disable_all_custom_nodes", None)
                ),
                "listen": primitive(getattr(cli_args, "listen", None)),
                "port": primitive(getattr(cli_args, "port", None)),
                "log_stdout": primitive(getattr(cli_args, "log_stdout", None)),
            },
            "measurement_contract": {
                "host_call_duration": "measured",
                "runtime_loaded_bytes_at_boundaries": "measured",
                "async_gpu_transfer_completion": "not measured",
                "per_layer_dynamic_vram_residency": "not measured",
            },
        }
    )


def serve_mode(argv: list[str]) -> int:
    if "--" not in argv:
        raise SystemExit("serve mode requires '--' before ComfyUI arguments")
    split = argv.index("--")
    ours = argv[:split]
    comfy_args = argv[split + 1 :]

    ap = argparse.ArgumentParser()
    ap.add_argument("--comfy-root", required=True, type=pathlib.Path)
    ap.add_argument("--event-log", required=True, type=pathlib.Path)
    args = ap.parse_args(ours)

    comfy_root = args.comfy_root.resolve()
    main_py = comfy_root / "main.py"
    if not main_py.is_file():
        raise SystemExit(f"ComfyUI main.py missing: {main_py}")

    os.chdir(comfy_root)
    sys.path.insert(0, str(comfy_root))
    sys.argv = [str(main_py), *comfy_args]

    writer = EventWriter(args.event_log.resolve())

    # Preserve ComfyUI's native initialization order. The observer does not
    # import model-management early. Instead it watches normal imports and
    # attaches only after both existing modules are fully available.
    import builtins

    original_import = builtins.__import__
    probe_installed = False

    def maybe_install_probe() -> None:
        nonlocal probe_installed
        if probe_installed:
            return
        mm = sys.modules.get("comfy.model_management")
        mp = sys.modules.get("comfy.model_patcher")
        if mm is None or mp is None:
            return
        if not hasattr(mm, "LoadedModel") or not hasattr(mp, "ModelPatcher"):
            return
        install_probe(mm, mp, writer)
        probe_installed = True
        builtins.__import__ = original_import

    def observing_import(name, globals=None, locals=None, fromlist=(), level=0):
        module = original_import(name, globals, locals, fromlist, level)
        maybe_install_probe()
        return module

    builtins.__import__ = observing_import
    writer.write(
        {
            "event": "comfy-launch",
            "comfy_root": str(comfy_root),
            "argv": sys.argv,
            "startup_order": "native-main.py-order",
        }
    )
    try:
        runpy.run_path(str(main_py), run_name="__main__")
    finally:
        builtins.__import__ = original_import

    if not probe_installed:
        raise RuntimeError("Prompt 7G observer never saw ComfyUI model-management modules")
    return 0


def load_json(path: pathlib.Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as fh:
        return json.load(fh)


def load_events(path: pathlib.Path) -> list[dict[str, Any]]:
    events = []
    with path.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if line:
                events.append(json.loads(line))
    return events


def summarize_durations(samples: list[float]) -> dict[str, Any]:
    if not samples:
        return {"count": 0}
    return {
        "count": len(samples),
        "min_ms": min(samples),
        "median_ms": statistics.median(samples),
        "max_ms": max(samples),
        "mean_ms": statistics.fmean(samples),
    }


def parse_gpu_telemetry(path: pathlib.Path) -> dict[str, Any]:
    rows = []
    if not path.is_file():
        return {"samples": 0}
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = [x.strip() for x in line.split(",")]
        if len(fields) != 6:
            continue
        try:
            rows.append(
                {
                    "timestamp": fields[0],
                    "memory_used_mib": float(fields[1]),
                    "memory_free_mib": float(fields[2]),
                    "utilization_gpu_percent": float(fields[3]),
                    "temperature_c": float(fields[4]),
                    "power_w": float(fields[5]),
                }
            )
        except ValueError:
            continue
    if not rows:
        return {"samples": 0}
    return {
        "samples": len(rows),
        "peak_memory_used_mib": max(x["memory_used_mib"] for x in rows),
        "minimum_memory_free_mib": min(x["memory_free_mib"] for x in rows),
        "peak_utilization_gpu_percent": max(x["utilization_gpu_percent"] for x in rows),
        "peak_temperature_c": max(x["temperature_c"] for x in rows),
        "peak_power_w": max(x["power_w"] for x in rows),
        "sampling_contract": "nvidia-smi device telemetry at approximately 100 ms; not per-transfer completion timing",
    }


def analyze_mode(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--event-log", required=True, type=pathlib.Path)
    ap.add_argument("--probe-oracle-summary", required=True, type=pathlib.Path)
    ap.add_argument("--baseline-oracle-summary", required=True, type=pathlib.Path)
    ap.add_argument("--census-summary", required=True, type=pathlib.Path)
    ap.add_argument("--gpu-telemetry", required=True, type=pathlib.Path)
    ap.add_argument("--output", required=True, type=pathlib.Path)
    args = ap.parse_args(argv)

    events = load_events(args.event_log)
    probe_oracle = load_json(args.probe_oracle_summary)
    baseline_oracle = load_json(args.baseline_oracle_summary)
    census = load_json(args.census_summary)

    baseline_hashes = {x.get("pixel_sha256") for x in baseline_oracle.get("runs", [])}
    probe_hashes = {x.get("pixel_sha256") for x in probe_oracle.get("runs", [])}
    if baseline_hashes != {EXPECTED_PIXEL_SHA256}:
        raise SystemExit(f"baseline oracle pixel identity changed: {baseline_hashes}")
    if probe_hashes != {EXPECTED_PIXEL_SHA256} or not probe_oracle.get("same_pixel_sha256"):
        raise SystemExit(f"probe oracle pixel identity changed: {probe_hashes}")

    installed = next((e for e in events if e.get("event") == "probe-installed"), None)
    if installed is None:
        raise SystemExit("Prompt 7G probe installation event missing")
    cli = installed.get("cli_contract", {})
    expected_cli = {
        "deterministic": True,
        "cache_none": True,
        "disable_all_custom_nodes": True,
        "log_stdout": True,
    }
    bad_cli = {k: (cli.get(k), v) for k, v in expected_cli.items() if cli.get(k) != v}
    if bad_cli:
        raise SystemExit(f"Prompt 7G ComfyUI CLI contract not active: {bad_cli}")

    end_calls = [
        e
        for e in events
        if e.get("event") == "call" and e.get("phase") == "end" and e.get("status") == "ok"
    ]

    by_component: dict[str, list[dict[str, Any]]] = {x: [] for x in REQUIRED_COMPONENTS}
    for event in end_calls:
        subject = event.get("subject_after") or event.get("subject_before") or {}
        label = subject.get("component_label")
        if label in by_component:
            by_component[label].append(event)

    missing = [name for name, items in by_component.items() if not items]
    if missing:
        raise SystemExit(f"Prompt 7G did not observe required component boundaries: {missing}")

    forced_unload = next(
        (
            event
            for event in reversed(end_calls)
            if event.get("call") == "model_management.unload_all_models"
        ),
        None,
    )
    if forced_unload is None:
        raise SystemExit("Prompt 7G did not observe the explicit post-oracle unload_all_models boundary")

    def labels_in_snapshot(snapshot):
        return {
            item.get("component_label")
            for item in (snapshot or [])
            if item.get("component_label") in REQUIRED_COMPONENTS
        }

    forced_before_labels = labels_in_snapshot(forced_unload.get("loaded_models_before"))
    forced_after_labels = labels_in_snapshot(forced_unload.get("loaded_models_after"))
    if forced_after_labels:
        raise SystemExit(
            "Prompt 7G forced /free left required components in ComfyUI's loaded-model "
            f"registry: {sorted(forced_after_labels)}"
        )

    component_summary: dict[str, Any] = {}
    for name, items in by_component.items():
        load_items = [
            e
            for e in items
            if e.get("call") in ("LoadedModel.model_load", "ModelPatcher.partially_load")
        ]
        unload_items = [
            e
            for e in items
            if e.get("call") in ("LoadedModel.model_unload", "ModelPatcher.partially_unload")
        ]
        if not load_items:
            raise SystemExit(f"Prompt 7G observed {name} but no load boundary")

        if unload_items:
            release_classification = "explicit-unload-boundary-observed"
            release_evidence = (
                "ComfyUI exposed one or more explicit model_unload/partially_unload "
                "callbacks for this component."
            )
        elif name not in forced_before_labels:
            release_classification = "released-before-forced-free"
            release_evidence = (
                "The component was loaded earlier but was already absent from ComfyUI's "
                "loaded-model registry when the explicit post-oracle unload_all_models "
                "boundary began. Exact release timing/mechanism is therefore not claimed."
            )
        else:
            raise SystemExit(
                f"Prompt 7G observed {name} present at forced /free but no explicit "
                "unload callback; observer coverage is insufficient"
            )

        component_summary[name] = {
            "load_host_call_duration": summarize_durations(
                [float(e["duration_ms"]) for e in load_items]
            ),
            "unload_host_call_duration": summarize_durations(
                [float(e["duration_ms"]) for e in unload_items]
            ),
            "release_classification": release_classification,
            "release_evidence": release_evidence,
            "present_at_forced_free_begin": name in forced_before_labels,
            "present_at_forced_free_end": name in forced_after_labels,
            "events": [
                {
                    "sequence": e.get("sequence"),
                    "call": e.get("call"),
                    "duration_ms": e.get("duration_ms"),
                    "result": e.get("result"),
                    "subject_before": e.get("subject_before"),
                    "subject_after": e.get("subject_after"),
                }
                for e in items
            ],
        }

    boundary_calls = {
        "model_management.load_models_gpu",
        "model_management.free_memory",
        "model_management.unload_all_models",
        "LoadedModel.model_load",
        "LoadedModel.model_unload",
    }
    residency_timeline = []
    for event in end_calls:
        if event.get("call") not in boundary_calls:
            continue
        residency_timeline.append(
            {
                "sequence": event.get("sequence"),
                "monotonic_ns": event.get("monotonic_ns"),
                "call": event.get("call"),
                "duration_ms": event.get("duration_ms"),
                "loaded_models_after": event.get("loaded_models_after", []),
                "device_free_bytes_after": event.get("device_free_bytes_after"),
            }
        )

    if not residency_timeline:
        raise SystemExit("Prompt 7G did not retain model-management residency snapshots")

    gpu_telemetry = parse_gpu_telemetry(args.gpu_telemetry)
    if int(gpu_telemetry.get("samples", 0)) <= 0:
        raise SystemExit("Prompt 7G retained no valid external GPU telemetry samples")

    output = {
        "schema": "air.prompt7g.residency-transition-summary.v1",
        "baseline_oracle_pixel_sha256": EXPECTED_PIXEL_SHA256,
        "probe_oracle_pixel_sha256": EXPECTED_PIXEL_SHA256,
        "probe_semantic_nonintrusion": True,
        "component_transition_observations": component_summary,
        "residency_timeline": residency_timeline,
        "gpu_telemetry": gpu_telemetry,
        "prompt7c_h_architectural_conclusion": census.get("architectural_conclusion"),
        "conditioning_runtime_shape_decision": {
            "status": "deferred-not-required-for-prompt8-boundary",
            "reason": (
                "Prompt 7 already established conditioning as a semantic value. "
                "Its exact storage shape/dtype is representation evidence, not required "
                "to define the workload-typed semantic boundary."
            ),
        },
        "observation_quality": {
            "component_model_management_boundaries": "measured",
            "component_load_host_call_duration": "measured",
            "component_unload_host_call_duration": (
                "measured only where ComfyUI exposed an explicit unload callback"
            ),
            "runtime_reported_loaded_bytes_at_boundaries": "measured",
            "forced_free_registry_terminal_state": "measured",
            "device_memory_trajectory": "sampled",
            "async_gpu_transfer_completion": "not directly measured",
            "per_layer_dynamic_vram_residency": "externally opaque",
        },
        "architectural_result": (
            "Prompt 8 requires identified prepared-resource/residency contracts under "
            "AIR's existing planning/capacity authorities. It does not require AIR to "
            "copy ComfyUI DynamicVRAM internals or introduce an image-specific residency manager."
        ),
        "remaining_prompt7_unknowns": [
            (
                "Exact per-layer asynchronous DynamicVRAM residency inside ComfyUI remains "
                "external-runtime implementation detail and must not be guessed."
            )
        ],
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print("PROMPT7G_RESIDENCY_TRANSITION_OBSERVABILITY=PASS")
    print(f"observed_components={','.join(REQUIRED_COMPONENTS)}")
    print(f"transition_events={len(end_calls)}")
    print(f"residency_timeline_points={len(residency_timeline)}")
    print(f"gpu_telemetry_samples={gpu_telemetry['samples']}")
    for component in REQUIRED_COMPONENTS:
        print(
            f"{component}_release="
            f"{component_summary[component]['release_classification']}"
        )
    print("conditioning_runtime_shape=DEFERRED_NOT_REQUIRED")
    print("async_gpu_transfer_completion=DIRECT_MEASUREMENT_NOT_CLAIMED")
    print("per_layer_dynamic_vram_residency=EXTERNALLY_OPAQUE")
    return 0


def main() -> int:
    if len(sys.argv) < 2 or sys.argv[1] not in ("serve", "analyze"):
        raise SystemExit("usage: prompt7g_residency_probe.py {serve|analyze} ...")
    mode = sys.argv[1]
    if mode == "serve":
        return serve_mode(sys.argv[2:])
    return analyze_mode(sys.argv[2:])


if __name__ == "__main__":
    raise SystemExit(main())
