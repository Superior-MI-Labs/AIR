#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import time
import urllib.error
import urllib.request
from PIL import Image

PROMPT_TEXT = (
    "A red enamel camping kettle on a weathered wooden table beside a cold "
    "northern lake at sunrise, realistic natural light, detailed metal and "
    "wood textures, atmospheric mist, no text."
)
SEED = 432262096973490
WIDTH = 1024
HEIGHT = 1024
STEPS = 4
CFG = 1.0
SAMPLER = "euler"

DIFFUSION = "flux-2-klein-4b-fp8.safetensors"
TEXT_ENCODER = "qwen_3_4b_fp4_flux2.safetensors"
VAE = "flux2-vae.safetensors"

SELECTED_NODE_TYPES = [
    "UNETLoader",
    "CLIPLoader",
    "CLIPTextEncode",
    "ConditioningZeroOut",
    "CFGGuider",
    "RandomNoise",
    "KSamplerSelect",
    "Flux2Scheduler",
    "EmptyFlux2LatentImage",
    "SamplerCustomAdvanced",
    "VAELoader",
    "VAEDecode",
    "SaveImage",
]


def request_json(url: str, *, data=None, timeout=30):
    body = None
    headers = {}
    if data is not None:
        body = json.dumps(data).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=body, headers=headers)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def build_prompt(filename_prefix: str):
    return {
        "1": {
            "class_type": "UNETLoader",
            "inputs": {
                "unet_name": DIFFUSION,
                "weight_dtype": "default",
            },
        },
        "2": {
            "class_type": "CLIPLoader",
            "inputs": {
                "clip_name": TEXT_ENCODER,
                "type": "flux2",
                "device": "default",
            },
        },
        "3": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "text": PROMPT_TEXT,
                "clip": ["2", 0],
            },
        },
        "4": {
            "class_type": "ConditioningZeroOut",
            "inputs": {
                "conditioning": ["3", 0],
            },
        },
        "5": {
            "class_type": "CFGGuider",
            "inputs": {
                "model": ["1", 0],
                "positive": ["3", 0],
                "negative": ["4", 0],
                "cfg": CFG,
            },
        },
        "6": {
            "class_type": "RandomNoise",
            "inputs": {
                "noise_seed": SEED,
            },
        },
        "7": {
            "class_type": "KSamplerSelect",
            "inputs": {
                "sampler_name": SAMPLER,
            },
        },
        "8": {
            "class_type": "Flux2Scheduler",
            "inputs": {
                "steps": STEPS,
                "width": WIDTH,
                "height": HEIGHT,
            },
        },
        "9": {
            "class_type": "EmptyFlux2LatentImage",
            "inputs": {
                "width": WIDTH,
                "height": HEIGHT,
                "batch_size": 1,
            },
        },
        "10": {
            "class_type": "SamplerCustomAdvanced",
            "inputs": {
                "noise": ["6", 0],
                "guider": ["5", 0],
                "sampler": ["7", 0],
                "sigmas": ["8", 0],
                "latent_image": ["9", 0],
            },
        },
        "11": {
            "class_type": "VAELoader",
            "inputs": {
                "vae_name": VAE,
            },
        },
        "12": {
            "class_type": "VAEDecode",
            "inputs": {
                "samples": ["10", 0],
                "vae": ["11", 0],
            },
        },
        "13": {
            "class_type": "SaveImage",
            "inputs": {
                "images": ["12", 0],
                "filename_prefix": filename_prefix,
            },
        },
    }


def wait_ready(base: str, timeout_s: float):
    deadline = time.monotonic() + timeout_s
    last = None
    while time.monotonic() < deadline:
        try:
            return request_json(base + "/system_stats", timeout=5)
        except Exception as exc:
            last = exc
            time.sleep(0.5)
    raise RuntimeError(f"ComfyUI did not become ready: {last}")


def wait_history(base: str, prompt_id: str, timeout_s: float):
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        data = request_json(base + f"/history/{prompt_id}", timeout=15)
        if prompt_id in data:
            entry = data[prompt_id]
            status = entry.get("status", {})
            status_str = status.get("status_str")
            if status_str in ("success", "error"):
                return entry
            if entry.get("outputs") and status_str is None:
                return entry
        time.sleep(0.5)
    raise RuntimeError(f"timed out waiting for ComfyUI prompt {prompt_id}")


def image_info(path: pathlib.Path):
    file_hash = hashlib.sha256(path.read_bytes()).hexdigest()
    with Image.open(path) as img:
        rgb = img.convert("RGB")
        pixel_hash = hashlib.sha256(rgb.tobytes()).hexdigest()
        size = [rgb.width, rgb.height]
        mode = rgb.mode
    return {
        "path": str(path),
        "file_sha256": file_hash,
        "pixel_sha256": pixel_hash,
        "width": size[0],
        "height": size[1],
        "mode": mode,
        "bytes": path.stat().st_size,
    }


def extract_output(history, output_dir: pathlib.Path):
    outputs = history.get("outputs", {})
    save = outputs.get("13") or outputs.get(13)
    if not save:
        raise RuntimeError("SaveImage output missing from ComfyUI history")
    images = save.get("images", [])
    if len(images) != 1:
        raise RuntimeError(f"expected one output image, got {len(images)}")
    item = images[0]
    subfolder = item.get("subfolder", "")
    filename = item["filename"]
    path = output_dir / subfolder / filename
    if not path.is_file():
        raise RuntimeError(f"ComfyUI output file not found: {path}")
    return path


def run_once(base: str, root: pathlib.Path, run_index: int, timeout_s: float):
    prefix = f"AIR-P7B-FLUX2-Oracle-run{run_index}"
    prompt = build_prompt(prefix)
    prompt_path = root / f"api-prompt-run{run_index}.json"
    prompt_path.write_text(json.dumps(prompt, indent=2, sort_keys=True) + "\n")

    start_wall = time.time()
    start = time.monotonic()
    response = request_json(base + "/prompt", data={"prompt": prompt}, timeout=30)
    if response.get("node_errors"):
        raise RuntimeError(
            "ComfyUI rejected oracle graph: "
            + json.dumps(response["node_errors"], sort_keys=True)
        )
    prompt_id = response.get("prompt_id")
    if not prompt_id:
        raise RuntimeError(f"ComfyUI did not return prompt_id: {response}")

    history = wait_history(base, prompt_id, timeout_s)
    elapsed = time.monotonic() - start
    end_wall = time.time()

    history_path = root / f"history-run{run_index}.json"
    history_path.write_text(json.dumps({prompt_id: history}, indent=2, sort_keys=True) + "\n")

    status = history.get("status", {})
    if status.get("status_str") == "error":
        raise RuntimeError(
            f"ComfyUI oracle run {run_index} failed: "
            + json.dumps(status, sort_keys=True)
        )

    output_path = extract_output(history, root / "output")
    info = image_info(output_path)
    info.update(
        {
            "run_index": run_index,
            "prompt_id": prompt_id,
            "elapsed_seconds": elapsed,
            "start_unix": start_wall,
            "end_unix": end_wall,
        }
    )
    return info


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base-url", required=True)
    ap.add_argument("--evidence-dir", required=True, type=pathlib.Path)
    ap.add_argument("--timeout", type=float, default=900.0)
    args = ap.parse_args()

    root = args.evidence_dir
    root.mkdir(parents=True, exist_ok=True)
    (root / "output").mkdir(parents=True, exist_ok=True)

    stats = wait_ready(args.base_url, 120.0)
    (root / "system-stats.json").write_text(
        json.dumps(stats, indent=2, sort_keys=True) + "\n"
    )

    object_info = request_json(args.base_url + "/object_info", timeout=60)
    selected = {
        key: object_info[key]
        for key in SELECTED_NODE_TYPES
        if key in object_info
    }
    missing = [x for x in SELECTED_NODE_TYPES if x not in selected]
    if missing:
        raise RuntimeError(f"required native ComfyUI node types missing: {missing}")
    (root / "object-info-selected.json").write_text(
        json.dumps(selected, indent=2, sort_keys=True) + "\n"
    )

    config = {
        "prompt": PROMPT_TEXT,
        "seed": SEED,
        "width": WIDTH,
        "height": HEIGHT,
        "steps": STEPS,
        "cfg": CFG,
        "sampler": SAMPLER,
        "diffusion": DIFFUSION,
        "text_encoder": TEXT_ENCODER,
        "vae": VAE,
        "cache_policy": "none",
        "deterministic_flag": True,
        "custom_nodes": "disabled",
    }
    (root / "oracle-config.json").write_text(
        json.dumps(config, indent=2, sort_keys=True) + "\n"
    )

    runs = [
        run_once(args.base_url, root, 1, args.timeout),
        run_once(args.base_url, root, 2, args.timeout),
    ]

    same_pixels = runs[0]["pixel_sha256"] == runs[1]["pixel_sha256"]
    same_files = runs[0]["file_sha256"] == runs[1]["file_sha256"]

    summary = {
        "schema": "air.prompt7b.external-oracle.v1",
        "oracle": "ComfyUI native FLUX.2 Klein 4B distilled text-to-image",
        "config": config,
        "runs": runs,
        "same_pixel_sha256": same_pixels,
        "same_file_sha256": same_files,
        "comparison_contract": (
            "bit-identical RGB pixels across two forced re-executions on the "
            "same pinned runtime/hardware"
        ),
    }
    (root / "oracle-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n"
    )

    print("=== PROMPT 7B EXTERNAL ORACLE ===")
    for run in runs:
        print(
            f"run{run['run_index']}: "
            f"elapsed_s={run['elapsed_seconds']:.3f} "
            f"size={run['width']}x{run['height']} "
            f"pixel_sha256={run['pixel_sha256']} "
            f"file_sha256={run['file_sha256']}"
        )
    print(f"same_pixel_sha256={'PASS' if same_pixels else 'FAIL'}")
    print(f"same_file_sha256={'PASS' if same_files else 'FAIL'}")

    if not same_pixels:
        raise SystemExit(20)

    print("PROMPT7B_EXTERNAL_ORACLE=PASS")


if __name__ == "__main__":
    main()
