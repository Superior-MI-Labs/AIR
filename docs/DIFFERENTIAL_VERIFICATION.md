# Differential Verification

`air-verify` compares the real reference and CUDA executors under one teacher-forced token history. It is an observer over production executor code, not a second transformer implementation.

## Decision verification

At each decision AIR compares:

- full logits;
- maximum/mean/RMS absolute error;
- top-1 token parity;
- top-k ranking;
- top1/top2 margin.

The reference top token is then forced into both executors for the next step. This preserves diagnostic value after a potential mismatch by preventing the two autoregressive histories from drifting apart.

## Stage tracing

Optional trace windows capture canonical internal boundaries such as embedding, normalized inputs, Q/K/V, post-RoPE values, attention output, residuals, FFN stages, final norm, and logits.

Tracing introduces CUDA synchronization/readback and must not be used for performance measurement.

## External llama.cpp comparison

`scripts/verify-llama-teacher.py` can feed exact numeric token arrays to llama.cpp's raw completion endpoint and compare next-token ranking/margins. llama.cpp does not expose AIR-compatible internal stage vectors, so external claims remain decision/top-k comparisons.

## Destruction coverage

The repository includes:

- randomized scalar-oracle tests for every supported quantized block format;
- a deterministic Qwen2 fixture that mixes all supported quantized execution formats;
- exact-token boundary tests around KV/prefill boundaries;
- context-edge and overflow tests.


## AIR 0.10.0 release qualification note

On the WolfCat-Studio CUDA 13.0.88 qualification environment, the historical
strict `--atol 0.001` gate is not reproducible even with frozen AIR 0.9.12.

For the qualified Qwen2.5-1.5B test model, AIR 0.10.0 and frozen AIR 0.9.12
produced the same 16-decision teacher-forced token history, full top-1 parity,
and identical reported per-decision error vectors.

The 0.10.0 release process therefore retains the strict 0.001 result as
diagnostic evidence and additionally requires a same-machine frozen-0.9.12
non-regression comparison. The current candidate must be finite, preserve the
same teacher-forced and top-token history, and be no worse than the frozen
baseline for every compared max, RMS, and mean absolute-error measurement
apart from a 1e-7 comparison epsilon.

This is a release qualification rule, not a claim that Reference and CUDA are
bitwise identical.
