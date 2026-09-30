#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import re
import subprocess
import sys
from typing import Any

EXPECTED_COMFY_HEAD = "986c4d154ef8c288382ac87d956b52a2b640c8b3"
EXPECTED_PIXEL_SHA256 = "c3a4278c608408df5019cf15162707e29263dee76e7a0114a6b1dcf2c29e1aa6"

DIFFUSION_SHA256 = "97ed34fe0567e436200f2faee3939b88f2b5d99f8af2a4dc16532c4245c0ccb6"
ENCODER_SHA256 = "3eab03a77adb0ee5304a4e677d5c10ac22f9049c1d7c894adca4f8bb39206ca8"
VAE_SHA256 = "868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3"

WIDTH = 1024
HEIGHT = 1024
STEPS = 4
SEED = 432262096973490


def load_json(path: pathlib.Path) -> Any:
    if not path.is_file():
        raise RuntimeError(f"missing evidence file: {path}")
    return json.loads(path.read_text())


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256_file(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def git_head(path: pathlib.Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(path), "rev-parse", "HEAD"],
        text=True,
    ).strip()


def source_excerpt(path: pathlib.Path, anchors: list[str], radius: int = 8) -> dict[str, Any]:
    text = path.read_text()
    lines = text.splitlines()
    hits = []
    for anchor in anchors:
        idx = next((i for i, line in enumerate(lines) if anchor in line), None)
        require(idx is not None, f"source anchor not found in {path}: {anchor}")
        lo = max(0, idx - radius)
        hi = min(len(lines), idx + radius + 1)
        hits.append(
            {
                "anchor": anchor,
                "line": idx + 1,
                "excerpt": "\n".join(lines[lo:hi]),
            }
        )
    return {
        "path": str(path),
        "sha256": sha256_file(path),
        "anchors": hits,
    }


def compute_flux2_schedule(width: int, height: int, steps: int) -> dict[str, Any]:
    image_seq_len = round(width * height / (16 * 16))
    a1, b1 = 8.73809524e-05, 1.89833333
    a2, b2 = 0.00016927, 0.45666666
    if image_seq_len > 4300:
        mu = a2 * image_seq_len + b2
    else:
        m_200 = a2 * image_seq_len + b2
        m_10 = a1 * image_seq_len + b1
        a = (m_200 - m_10) / 190.0
        b = m_200 - 200.0 * a
        mu = a * steps + b

    e_mu = math.exp(mu)
    raw = [1.0 - i / steps for i in range(steps + 1)]
    shifted = []
    for t in raw:
        if t == 0.0:
            shifted.append(0.0)
        else:
            shifted.append(e_mu / (e_mu + (1.0 / t - 1.0)))
    return {
        "image_seq_len": image_seq_len,
        "mu": mu,
        "raw_timesteps": raw,
        "sigmas": shifted,
        "transitions": steps,
    }


def parse_staged_mb(log_text: str, model_name: str) -> int | None:
    pattern = rf"Model {re.escape(model_name)} prepared for dynamic VRAM loading\. ([0-9]+)MB Staged"
    m = re.search(pattern, log_text)
    return int(m.group(1)) if m else None


def parse_identity(path: pathlib.Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in path.read_text().splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Derive AIR Prompt 7C-7H evidence census from the qualified FLUX.2 oracle"
    )
    ap.add_argument("oracle_dir", type=pathlib.Path)
    ap.add_argument("comfy_root", type=pathlib.Path)
    ap.add_argument("air_root", type=pathlib.Path)
    ap.add_argument("output_dir", type=pathlib.Path)
    args = ap.parse_args()

    oracle = args.oracle_dir.resolve()
    comfy = args.comfy_root.resolve()
    air = args.air_root.resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    summary = load_json(oracle / "oracle-summary.json")
    telemetry = load_json(oracle / "telemetry-summary.json")
    prompt = load_json(oracle / "api-prompt-run1.json")
    prompt2 = load_json(oracle / "api-prompt-run2.json")
    selection = load_json(oracle / "selection-evidence" / "safetensors-census.json")
    identity = parse_identity(oracle / "identity.txt")
    log_text = (oracle / "comfy-server.log").read_text()

    require(summary.get("same_pixel_sha256") is True, "oracle pixel reproducibility did not pass")
    require(len(summary.get("runs", [])) == 2, "oracle summary does not contain two runs")
    require(
        all(r.get("pixel_sha256") == EXPECTED_PIXEL_SHA256 for r in summary["runs"]),
        "oracle pixel identity changed",
    )
    require(prompt == prompt2, "forced oracle API graphs differ between run1 and run2")
    require(identity.get("comfy_head") == EXPECTED_COMFY_HEAD, "oracle pinned ComfyUI head changed")
    require(git_head(comfy) == EXPECTED_COMFY_HEAD, "local ComfyUI checkout no longer matches oracle head")

    artifact_lines = (oracle / "selection-evidence" / "artifact-sha256.txt").read_text()
    for digest in (DIFFUSION_SHA256, ENCODER_SHA256, VAE_SHA256):
        require(digest in artifact_lines, f"qualified artifact identity missing from retained evidence: {digest}")

    node_types = {node["class_type"] for node in prompt.values()}
    expected_nodes = {
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
    }
    require(node_types == expected_nodes, f"oracle graph node set changed: {sorted(node_types)}")

    require(prompt["1"]["inputs"]["unet_name"] == "flux-2-klein-4b-fp8.safetensors", "diffusion node changed")
    require(prompt["2"]["inputs"]["clip_name"] == "qwen_3_4b_fp4_flux2.safetensors", "text encoder node changed")
    require(prompt["2"]["inputs"]["type"] == "flux2", "text encoder semantic type changed")
    require(prompt["6"]["inputs"]["noise_seed"] == SEED, "oracle seed changed")
    require(prompt["7"]["inputs"]["sampler_name"] == "euler", "oracle sampler changed")
    require(prompt["8"]["inputs"] == {"steps": STEPS, "width": WIDTH, "height": HEIGHT}, "oracle scheduler changed")
    require(prompt["9"]["inputs"] == {"width": WIDTH, "height": HEIGHT, "batch_size": 1}, "oracle latent changed")
    require(prompt["11"]["inputs"]["vae_name"] == "flux2-vae.safetensors", "oracle VAE changed")

    source = {
        "nodes_flux": source_excerpt(
            comfy / "comfy_extras" / "nodes_flux.py",
            [
                "class EmptyFlux2LatentImage",
                "latent = torch.zeros([batch_size, 128, height // 16, width // 16]",
                "def generalized_time_snr_shift",
                "def compute_empirical_mu",
                "def get_schedule",
                "class Flux2Scheduler",
                "seq_len = (width * height / (16 * 16))",
            ],
        ),
        "custom_sampler": source_excerpt(
            comfy / "comfy_extras" / "nodes_custom_sampler.py",
            [
                "class Noise_RandomNoise",
                "return comfy.sample.prepare_noise(latent_image, self.seed, batch_inds)",
                "class CFGGuider",
                "class RandomNoise",
                "class SamplerCustomAdvanced",
                "samples = guider.sample(noise.generate_noise(latent)",
            ],
        ),
        "nodes": source_excerpt(
            comfy / "nodes.py",
            [
                "class CLIPTextEncode",
                "class ConditioningZeroOut",
                "class UNETLoader",
                "class CLIPLoader",
                "class VAELoader",
                "class VAEDecode",
                "class SaveImage",
            ],
        ),
        "sd": source_excerpt(
            comfy / "comfy" / "sd.py",
            [
                "elif te_model == TEModel.QWEN3_4B:",
                "clip_target.tokenizer = comfy.text_encoders.flux.KleinTokenizer",
            ],
        ),
        "air_execution": source_excerpt(
            air / "include" / "air" / "execution.hpp",
            [
                "struct RequestProfile {",
                "std::uint64_t prompt_tokens{0};",
                "struct RuntimeSnapshot {",
                "std::uint64_t resident_kv_bytes{0};",
                "struct ExecutionPlan {",
                "KvPolicy kv{};",
                "LinearPolicy linear{};",
                "AttentionPolicy attention{};",
                "enum class PhysicalInvocationKind",
                "enum class ExecutionPayloadKind",
            ],
        ),
        "air_serving": source_excerpt(
            air / "include" / "air" / "serving.hpp",
            [
                "struct InferenceRequest {",
                "std::string prompt;",
                "std::vector<ChatMessage> messages;",
                "std::uint64_t prompt_tokens{0};",
                "std::uint64_t generated_tokens{0};",
                "std::uint64_t kv_bytes{0};",
            ],
        ),
    }
    (out / "source-evidence.json").write_text(json.dumps(source, indent=2, sort_keys=True) + "\n")

    schedule = compute_flux2_schedule(WIDTH, HEIGHT, STEPS)
    latent_shape = [1, 128, HEIGHT // 16, WIDTH // 16]
    latent_elements = math.prod(latent_shape)

    staged = {
        "text_encoder_mb": parse_staged_mb(log_text, "Flux2TEModel_"),
        "denoiser_mb": parse_staged_mb(log_text, "Flux2"),
        "vae_mb": parse_staged_mb(log_text, "AutoencoderKL"),
    }
    require(all(v is not None for v in staged.values()), f"missing one or more staged-size observations: {staged}")

    components = {
        "schema": "air.prompt7.component-census.v1",
        "components": [
            {
                "id": "text-tokenizer",
                "role": "prompt string -> token representation",
                "authority": "pinned ComfyUI KleinTokenizer selected by FLUX2 + QWEN3_4B",
                "artifact": None,
                "state": "runtime/code-defined",
                "evidence_class": "Fact",
            },
            {
                "id": "text-encoder",
                "role": "token representation -> conditioning",
                "artifact_sha256": ENCODER_SHA256,
                "artifact_storage": "FP4/quantized safetensors package observed in Stage 7A",
                "runtime_load_device": "cuda:0",
                "runtime_offload_device": "cpu",
                "runtime_initial_current": "cpu",
                "staged_mb": staged["text_encoder_mb"],
                "independently_loadable": True,
                "evidence_class": "Fact+Measurement",
            },
            {
                "id": "flux2-denoiser",
                "role": "iterative latent transformation conditioned by text and schedule",
                "artifact_sha256": DIFFUSION_SHA256,
                "artifact_storage": "FP8/mixed safetensors package observed in Stage 7A",
                "staged_mb": staged["denoiser_mb"],
                "independently_loadable": True,
                "evidence_class": "Fact+Measurement",
            },
            {
                "id": "flux2-scheduler",
                "role": "resolution/step count -> sigma schedule",
                "resource_identity": f"ComfyUI@{EXPECTED_COMFY_HEAD}:comfy_extras/nodes_flux.py",
                "schedule": schedule,
                "independently_loadable": False,
                "evidence_class": "Fact",
            },
            {
                "id": "seeded-noise",
                "role": "seed + latent shape -> noise tensor",
                "seed": SEED,
                "rng_realization_point": "SamplerCustomAdvanced via Noise_RandomNoise.generate_noise",
                "evidence_class": "Fact",
            },
            {
                "id": "latent-state",
                "role": "initial and iteratively updated image latent",
                "shape": latent_shape,
                "elements": latent_elements,
                "source_dtype_policy": "torch.zeros call does not specify dtype; physical dtype follows runtime default/intermediate handling",
                "persistent_across_iterations": True,
                "evidence_class": "Fact+Inference",
            },
            {
                "id": "vae",
                "role": "final latent -> RGB image",
                "artifact_sha256": VAE_SHA256,
                "runtime_load_device": "cuda:0",
                "runtime_offload_device": "cpu",
                "runtime_dtype": "torch.bfloat16",
                "staged_mb": staged["vae_mb"],
                "independently_loadable": True,
                "evidence_class": "Fact+Measurement",
            },
            {
                "id": "output-image",
                "role": "decoded semantic result",
                "shape": [1, HEIGHT, WIDTH, 3],
                "semantic_identity": "decoded RGB pixel bytes",
                "pixel_sha256": EXPECTED_PIXEL_SHA256,
                "container_identity": "not semantic for this oracle; PNG hashes differed",
                "evidence_class": "Measurement+Policy",
            },
        ],
    }
    (out / "component-census.json").write_text(json.dumps(components, indent=2, sort_keys=True) + "\n")

    iteration = {
        "schema": "air.prompt7.iteration-state-census.v1",
        "carried_value": {
            "kind": "latent",
            "shape": latent_shape,
            "semantic_role": "current image state",
        },
        "schedule": schedule,
        "iteration_count": STEPS,
        "sampler": "euler",
        "cfg": 1.0,
        "conditioning": {
            "positive": "fixed for the sampler invocation",
            "negative": "ConditioningZeroOut(positive), fixed for the sampler invocation",
        },
        "stochastic_state": {
            "seed": SEED,
            "noise_realized_once_before iterative guider.sample": True,
        },
        "loop_boundary": {
            "source_operation": "SamplerCustomAdvanced -> guider.sample",
            "termination": "completion of sigma schedule transitions",
        },
        "recomputation_observation": {
            "result_cache_disabled": True,
            "two_forced_executions": True,
            "same_pixels": True,
        },
        "unknowns": [
            "The retained oracle does not expose every per-step latent tensor or its runtime dtype.",
            "Per-step denoiser wall time was not retained separately from the sampler progress/total execution.",
        ],
    }
    (out / "iteration-state-census.json").write_text(json.dumps(iteration, indent=2, sort_keys=True) + "\n")

    values = {
        "schema": "air.prompt7.semantic-value-census.v1",
        "values": [
            {"kind": "prompt-text", "meaning": "user conditioning request", "mutability": "immutable per request"},
            {"kind": "conditioning", "meaning": "encoded prompt state consumed by guider/denoiser", "mutability": "immutable during sampling"},
            {"kind": "seed", "meaning": "identity of stochastic initialization", "mutability": "immutable per request"},
            {"kind": "noise", "meaning": "seed-realized stochastic tensor matched to latent geometry", "mutability": "realized once per execution"},
            {"kind": "sigma-schedule", "meaning": "ordered iterative schedule controlling denoising trajectory", "shape": [STEPS + 1], "values": schedule["sigmas"], "mutability": "immutable schedule"},
            {"kind": "latent", "meaning": "image state in latent space carried through iterations", "shape": latent_shape, "mutability": "evolves across sampling"},
            {"kind": "image", "meaning": "decoded RGB result", "shape": [HEIGHT, WIDTH, 3], "mutability": "terminal output"},
        ],
        "separation_rule": "semantic kind is independent of storage dtype, tensor layout, and device placement",
    }
    (out / "semantic-value-census.json").write_text(json.dumps(values, indent=2, sort_keys=True) + "\n")

    operations = {
        "schema": "air.prompt7.operation-boundary.v1",
        "semantic_operations_for_this_oracle": [
            {"operation": "encode-conditioning", "inputs": ["prompt-text"], "outputs": ["conditioning"]},
            {"operation": "derive-negative-conditioning", "inputs": ["conditioning"], "outputs": ["conditioning"]},
            {"operation": "initialize-latent", "inputs": ["width", "height", "batch"], "outputs": ["latent"]},
            {"operation": "derive-schedule", "inputs": ["steps", "width", "height"], "outputs": ["sigma-schedule"]},
            {"operation": "realize-seeded-noise", "inputs": ["seed", "latent geometry"], "outputs": ["noise"]},
            {"operation": "iterative-sample", "inputs": ["latent", "noise", "conditioning", "sigma-schedule", "sampler", "cfg"], "outputs": ["latent"]},
            {"operation": "decode-latent", "inputs": ["latent"], "outputs": ["image"]},
        ],
        "component_calls": [
            "Klein tokenizer/text encoder",
            "Flux2 denoiser repeatedly inside iterative sampling",
            "Flux2 scheduler",
            "VAE decode",
        ],
        "semantically_observable_configuration": [
            "seed",
            "resolution",
            "step count",
            "sigma schedule",
            "Euler sampler selection",
            "CFG value",
            "model/component artifact identities",
        ],
        "physical_not_semantic_truth": [
            "CUDA versus CPU placement",
            "DynamicVRAM staging/offload policy",
            "async offload stream count",
            "PyTorch attention implementation",
            "FP8/FP4 storage mechanics as a universal semantic type",
            "kernel launch and transfer scheduling",
        ],
        "policy": "Do not freeze these oracle-specific operations as a universal AIR operation catalog in Prompt 7.",
    }
    (out / "operation-boundary.json").write_text(json.dumps(operations, indent=2, sort_keys=True) + "\n")

    resources = {
        "schema": "air.prompt7.resource-residency-census.v1",
        "observed": {
            "vram_mode": "NORMAL_VRAM",
            "dynamic_vram": True,
            "async_weight_offload_streams": 2,
            "component_staged_mb": staged,
            "peak_gpu_memory_used_mib": telemetry["peak_gpu_memory_used_mib"],
            "minimum_gpu_memory_free_mib": telemetry["minimum_gpu_memory_free_mib"],
            "peak_gpu_utilization_percent": telemetry["peak_gpu_utilization_percent"],
            "peak_server_rss_mib": telemetry["peak_server_rss_mib"],
            "minimum_host_mem_available_gib": telemetry["minimum_host_mem_available_gib"],
        },
        "inferences": [
            "The workflow has independently meaningful component residency lifetimes: text encoder, denoiser, and VAE are separate load/offload resources.",
            "Peak device memory exceeds any one staged component, so transient activations/workspaces/overlap materially contribute to device demand.",
            "The current 16 GiB device has substantial headroom for this oracle under ComfyUI DynamicVRAM, but that does not prove an all-components-resident plan is legal or optimal.",
        ],
        "not_observed_precisely": [
            "per-component load latency",
            "per-component eviction latency",
            "exact device-resident bytes for each component at each phase",
            "peak transient bytes attributable to a specific operation",
        ],
    }
    (out / "resource-residency-census.json").write_text(json.dumps(resources, indent=2, sort_keys=True) + "\n")

    gaps = {
        "schema": "air.prompt7.current-air-gap-map.v1",
        "gaps": [
            {
                "requirement": "non-token request shape: resolution, steps, batch, seed, sampler/configuration",
                "current_air": "RequestProfile contains prompt_tokens, max_output_tokens, active_sequences; InferenceRequest is prompt/messages/generation",
                "status": "missing",
                "smallest_evolution": "add a workload-typed request/profile extension under the existing planner/serving authority; do not replace Qwen request contracts",
            },
            {
                "requirement": "semantic values beyond token sequences: conditioning, noise, schedule, latent, image",
                "current_air": "production serving/result contracts are token/text oriented",
                "status": "missing",
                "smallest_evolution": "introduce typed workload semantic values only where Qwen+FLUX evidence requires them; avoid universal tensor IR",
            },
            {
                "requirement": "iterative state transformation controlled by a schedule",
                "current_air": "scheduler understands prefill/decode token work; ExecutionGraph R0 describes already-concrete token physical invocations",
                "status": "missing",
                "smallest_evolution": "represent a workload-level iterative region/state contract that can lower through the one AIR runtime without creating an image scheduler",
            },
            {
                "requirement": "multiple independently resident model components",
                "current_air": "RuntimeSnapshot exposes resident_kv_bytes plus aggregate prepared_artifact_bytes",
                "status": "partially-reusable",
                "smallest_evolution": "generalize resource/residency facts to identified prepared resources while retaining KV as Qwen-specific resource data",
            },
            {
                "requirement": "workload-specific legal physical implementations",
                "current_air": "ExecutionPlan tactic vocabulary is Qwen transformer prefill/decode linear+attention+KV",
                "status": "transformer-specific",
                "smallest_evolution": "preserve one ExecutionPlan authority but add a workload-scoped physical plan payload/typed realization rather than image-specific parallel planning",
            },
            {
                "requirement": "physical graph observation for non-token work",
                "current_air": "ExecutionGraph authority/placement/observation concepts are reusable; invocation kinds and payload enums are token/Qwen specific",
                "status": "partially-reusable",
                "smallest_evolution": "retain graph authority and topology identity; extend physical work/payload vocabulary only from Prompt 8 lowering evidence",
            },
            {
                "requirement": "admission/capacity across component residency and transient memory",
                "current_air": "CapacityScheduler authority exists but current resource accounting is dominated by KV and prepared artifact aggregates",
                "status": "partially-reusable",
                "smallest_evolution": "feed component/transient resource requirements into the existing capacity authority; do not create an image memory manager",
            },
            {
                "requirement": "workload output semantics and metrics",
                "current_air": "RequestMetrics/InferenceResponse expose prompt/generated tokens, prefill/decode/TTFT and text/tokens",
                "status": "missing",
                "smallest_evolution": "add workload-specific result/phase metrics while preserving common queue/plan/total timing and evidence semantics",
            },
            {
                "requirement": "hardware topology/environment and evidence provenance",
                "current_air": "already separated and qualified in Prompts 1-3",
                "status": "reusable",
                "smallest_evolution": "reuse unchanged",
            },
            {
                "requirement": "single strategy/planner/resource ownership",
                "current_air": "Strategy Lab + PreparedModel + CapacityScheduler + InferenceService authorities already exist",
                "status": "reusable-as-authority",
                "smallest_evolution": "extend those authorities for the second workload; do not introduce diffusion-specific peers",
            },
        ],
    }
    (out / "current-air-gap-map.json").write_text(json.dumps(gaps, indent=2, sort_keys=True) + "\n")

    unresolved = {
        "schema": "air.prompt7.unresolved-evidence.v1",
        "items": [
            {
                "id": "component-transition-timing",
                "stage": "7G",
                "need": "measure text-encoder, denoiser, and VAE transition/load/offload timing separately if the external runtime can expose it without invasive instrumentation",
                "reason": "7B retained staged sizes and aggregate runtime but not per-component transition durations",
            },
            {
                "id": "component-residency-timeline",
                "stage": "7G",
                "need": "characterize per-phase component device residency or explicitly retain it as externally opaque",
                "reason": "DynamicVRAM logs prove independently managed resources but do not provide an exact residency timeline",
            },
            {
                "id": "conditioning-runtime-shape",
                "stage": "7C/7E",
                "need": "retain exact conditioning tensor shape/dtype only if it becomes necessary to define Prompt 8's semantic boundary",
                "reason": "semantic identity is established, but tensor representation was not required by the 7B oracle contract",
            },
        ],
        "policy": "Unknown representation details must remain unknown unless they materially constrain Prompt 8 architecture.",
    }
    (out / "unresolved-evidence.json").write_text(json.dumps(unresolved, indent=2, sort_keys=True) + "\n")

    summary_out = {
        "schema": "air.prompt7.census-summary.v1",
        "oracle_pixel_sha256": EXPECTED_PIXEL_SHA256,
        "schedule": schedule,
        "latent_shape": latent_shape,
        "component_staged_mb": staged,
        "peak_gpu_memory_used_mib": telemetry["peak_gpu_memory_used_mib"],
        "same_machine_reproducibility": "bit-identical RGB pixels",
        "gap_counts": {
            status: sum(1 for g in gaps["gaps"] if g["status"] == status)
            for status in sorted({g["status"] for g in gaps["gaps"]})
        },
        "unresolved_count": len(unresolved["items"]),
        "architectural_conclusion": (
            "The second workflow falsifies a token-only universal architecture but does not require "
            "a second runtime. Existing hardware, planning authority, capacity authority, resource "
            "ownership, and evidence concepts remain reusable; request/value/iteration/resource "
            "vocabularies require workload-typed evolution."
        ),
    }
    (out / "prompt7c-h-summary.json").write_text(json.dumps(summary_out, indent=2, sort_keys=True) + "\n")

    checksums = out / "SHA256SUMS.txt"
    with checksums.open("w") as fh:
        for path in sorted(p for p in out.rglob("*") if p.is_file() and p.name != checksums.name):
            fh.write(f"{sha256_file(path)}  {path.relative_to(out)}\n")

    print("PROMPT7C_H_ORACLE_ANALYSIS=PASS")
    print(f"oracle_pixel_sha256={EXPECTED_PIXEL_SHA256}")
    print(f"latent_shape={latent_shape}")
    print("flux2_sigmas=" + ",".join(f"{x:.9f}" for x in schedule["sigmas"]))
    print(f"text_encoder_staged_mb={staged['text_encoder_mb']}")
    print(f"denoiser_staged_mb={staged['denoiser_mb']}")
    print(f"vae_staged_mb={staged['vae_mb']}")
    print(f"peak_gpu_memory_used_mib={telemetry['peak_gpu_memory_used_mib']}")
    print(f"unresolved_evidence_items={len(unresolved['items'])}")
    print("next_falsification_target=Prompt 7G focused residency/transition observability")
    print(f"Evidence directory: {out}")


if __name__ == "__main__":
    main()
