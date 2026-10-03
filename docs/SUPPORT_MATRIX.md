# AIR 0.11.0 Support Matrix

## Production inference model scope

| Area | Qualified / supported |
|---|---|
| Container | GGUF v3 |
| Production architecture metadata | `qwen2` |
| Tested production family | Qwen2.5 GGUF |
| Tokenizer | byte-level GPT-2 BPE / Qwen2 pre-tokenization |
| Chat rendering | explicit Qwen2 ChatML |
| Reference execution | hosted exact-source qualified |
| NVIDIA CUDA execution | backend present; retained pre-R1 machine evidence; final exact-source post-R1 replay pending |

Qwen2 remains the only AIR-owned end-to-end production model executor in
0.11.0.

## Adaptive execution foundation

Qualified hosted contracts:

- HardwareTopology stable structural identity;
- ExecutionEnvironmentSnapshot dynamic availability;
- typed execution spans/timeline;
- ExecutionGraph R1;
- workload structure: autoregressive tokens / iterative state;
- work units: tokens / iterations / bytes;
- identified prepared-resource requirements/residency;
- evidence-backed Strategy Lab planning;
- trusted semantic implementation registry;
- structured MissingSemantic results;
- AIR Control Room Web 3.3.

## FLUX.2 Klein second architecture

| Capability | 0.11 status |
|---|---|
| Frozen external oracle characterization | qualified from retained Prompt 7 evidence |
| Semantic value/operation contract | hosted qualified |
| Iterative invocation structure | hosted qualified |
| Text encoder / denoiser / VAE resource identities | hosted qualified |
| Shared component/resource physical plan | hosted qualified |
| Descriptive ExecutionGraph R1 projection | hosted qualified |
| Latent geometry derivation | AIR-owned + hosted qualified |
| Sigma/schedule derivation | AIR-owned + hosted qualified |
| Text encoder execution in AIR | not implemented / missing semantic |
| Denoiser execution in AIR | not implemented / missing semantic |
| VAE execution in AIR | not implemented / missing semantic |
| AIR-owned decoded image parity | not qualified |

The FLUX graph is descriptive where AIR lacks an implementation. It is not a
parallel diffusion runtime.

## Qwen2 tensor encodings

Reference and CUDA executors support the retained qualified tensor encodings:

- F32
- F16
- BF16
- Q4_0
- Q5_0
- Q8_0
- Q4_K
- Q6_K

Unsupported encodings fail explicitly.

## Qwen2 semantic constraints

Qualified:

- grouped-query attention;
- optional Q/K/V biases;
- RMSNorm;
- SwiGLU FFN;
- full even head-dimension RoPE;
- tied output embeddings when `output.weight` is absent.

Not claimed:

- arbitrary model architectures;
- sliding-window attention;
- partial rotary dimensions;
- arbitrary RoPE scaling semantics;
- universal neural/tensor IR.

## Backends

### Reference

- CPU correctness path;
- hosted exact-source 0.11 qualification;
- paged KV and sequence state;
- generation and Decision;
- detailed observation and ExecutionGraph R1 evidence.

### NVIDIA CUDA

AIR retains:

- native multi-token prefill;
- executor-owned paged KV;
- demand allocation/page reuse;
- deterministic device greedy selection;
- specialized matrix kernels;
- identified optional prepared resources;
- evidence-backed dense-FP32 research tactic.

Final-source 0.11 post-R1 CUDA graph concordance, memory pressure, power and
thermal behavior remain unqualified because the development GPU became
unavailable during final hardening.

## Platforms

Hosted final qualification:

- Ubuntu 24.04 x86-64;
- CPU/reference execution.

Retained development evidence:

- Linux Mint;
- NVIDIA RTX 3080 Laptop GPU;
- CUDA execution through the pre-final adaptive stages.

AIR does not claim a tested macOS or Windows release matrix.

## Human usability

Control Room Web 3.3 passes static/installed hosted checks. A final interactive
human novice/research usability session remains release evidence debt.
