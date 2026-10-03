#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build-adaptive-preflight}"
PORT="${2:-18931}"
EXPECTED_VERSION="0.11.0"
EXPECTED_BRANCH="architecture/adaptive-execution-substrate-r0"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_RELEASE_OUT:-${RUNNER_TEMP:-/tmp}/AIR-0.11.0-RC-$STAMP}"
HOSTED="$OUT/hosted"
PREFIX="$HOSTED/prefix"

cd "$ROOT"

BRANCH="$(git branch --show-current)"
HEAD="$(git rev-parse HEAD)"
TREE="$(git rev-parse HEAD^{tree})"

[[ "$BRANCH" == "$EXPECTED_BRANCH" ]] || {
    echo "ERROR: release qualification requires $EXPECTED_BRANCH; current=$BRANCH" >&2
    exit 2
}
[[ -z "$(git status --porcelain)" ]] || {
    echo "ERROR: release qualification requires a clean worktree" >&2
    git status --short >&2
    exit 2
}

VERSION="$(sed -nE 's/^project\(AIR VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt | head -n1)"
[[ "$VERSION" == "$EXPECTED_VERSION" ]] || {
    echo "ERROR: project version=$VERSION expected=$EXPECTED_VERSION" >&2
    exit 2
}

for file in     README.md     docs/RELEASE-0.11.0.md     docs/RELEASE-PROVENANCE.md     docs/SUPPORT_MATRIX.md     docs/PUBLIC_CONTRACTS.md     docs/ARCHITECTURE.md     docs/adaptive-execution/PROMPT-11.md     docs/adaptive-execution/PROMPT-12.md; do
    [[ -f "$file" ]] || {
        echo "ERROR: missing release file: $file" >&2
        exit 2
    }
done

grep -Fq 'AIR 0.11.0' README.md
grep -Fq 'Adaptive Execution Foundation' docs/RELEASE-0.11.0.md
grep -Fq 'post-R1 Qwen CUDA graph concordance' docs/RELEASE-0.11.0.md
grep -Fq 'not implemented / missing semantic' docs/SUPPORT_MATRIX.md
grep -Fq 'ExecutionGraph R1 schema version is `3`' docs/PUBLIC_CONTRACTS.md
echo "release_static_contract_checks=PASS"

mkdir -p "$OUT"
{
    echo "date=$(date -Is)"
    echo "version=$VERSION"
    echo "branch=$BRANCH"
    echo "head=$HEAD"
    echo "tree=$TREE"
} > "$OUT/release-identity.txt"

echo "=== AIR 0.11.0 ADAPTIVE PREFLIGHT ==="
AIR_PREFLIGHT_OUT="$BUILD_DIR" bash scripts/preflight-adaptive.sh     2>&1 | tee "$OUT/preflight-terminal.txt"

echo
echo "=== AIR 0.11.0 HOSTED INSTALLED-PRODUCT HARDENING ==="
AIR_P11_HOSTED_OUT="$HOSTED"     bash scripts/qualify-adaptive-prompt11-hosted.sh "$BUILD_DIR" "$PORT"     2>&1 | tee "$OUT/hosted-terminal.txt"

grep -Fq 'PROMPT11_HOSTED_RC_HARDENING=PASS' "$OUT/hosted-terminal.txt"

echo
echo "=== INSTALLED VERSION SURFACES ==="
: > "$OUT/version-surfaces.txt"
for tool in air-cli air-server air-bench air-qualify; do
    value="$("$PREFIX/bin/$tool" --version 2>&1 | head -n1)"
    printf '%s=%s\n' "$tool" "$value" | tee -a "$OUT/version-surfaces.txt"
    grep -Fq "$EXPECTED_VERSION" <<<"$value"
done

verify_help="$("$PREFIX/bin/air-verify" --help 2>&1)"
printf 'air-verify=%s\n' "$(head -n1 <<<"$verify_help")" >> "$OUT/version-surfaces.txt"
grep -Fq "$EXPECTED_VERSION" <<<"$verify_help"

strategy_help="$("$PREFIX/bin/air-strategy-probe" --help 2>&1 || true)"
printf 'air-strategy-probe=%s\n' "$(head -n1 <<<"$strategy_help")" >> "$OUT/version-surfaces.txt"

version_file="$PREFIX/lib/cmake/AIR/AIRConfigVersion.cmake"
grep -Fq 'set(PACKAGE_VERSION "0.11.0")' "$version_file"
echo 'cmake_package_version=0.11.0' >> "$OUT/version-surfaces.txt"

echo
echo "=== RELEASE ARCHIVES ==="
SOURCE="$OUT/AIR-0.11.0-source.tar.gz"
git archive --format=tar --prefix="AIR-0.11.0/" "$HEAD" | gzip -n -9 > "$SOURCE"

HOSTED_ARCHIVE="$OUT/AIR-0.11.0-hosted-qualification-evidence.tar.gz"
tar --sort=name --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner     -C "$HOSTED" -cf - . | gzip -n -9 > "$HOSTED_ARCHIVE"

cp docs/RELEASE-0.11.0.md "$OUT/RELEASE-NOTES.md"
cp docs/RELEASE-PROVENANCE.md "$OUT/RELEASE-PROVENANCE.md"
cp docs/SUPPORT_MATRIX.md "$OUT/SUPPORT-MATRIX.md"
cp docs/PUBLIC_CONTRACTS.md "$OUT/PUBLIC-CONTRACTS.md"

python3 - "$OUT" "$HEAD" "$TREE" "$VERSION" "$SOURCE" "$HOSTED_ARCHIVE" <<'PY'
import hashlib
import json
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
head, tree, version = sys.argv[2:5]
source = pathlib.Path(sys.argv[5])
hosted = pathlib.Path(sys.argv[6])

def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

manifest = {
    "schema": "air.release.v2",
    "release": "AIR 0.11.0 — Adaptive Execution Foundation",
    "version": version,
    "git_head": head,
    "git_tree": tree,
    "artifacts": {
        "source": {"name": source.name, "sha256": sha(source)},
        "hosted_qualification": {"name": hosted.name, "sha256": sha(hosted)},
    },
    "qualification": {
        "hosted_reference_exact_source": True,
        "web_3_3_hosted": True,
        "ctest_count": 19,
        "installed_cmake_consumer": True,
        "http_generation": True,
        "http_decision": True,
        "execution_graph_r1_reference": True,
        "semantic_registry": True,
        "server_restart": True,
        "qwen_cuda_retained_prior_machine_evidence": True,
        "qwen_cuda_post_r1_exact_source": False,
        "nvidia_final_source_pressure_thermal_power": False,
        "flux2_shared_structure": True,
        "flux2_air_deterministic_semantics": True,
        "flux2_air_model_component_execution": False,
        "flux2_image_parity": False,
        "human_control_room_usability": False,
    },
    "claims": {
        "universal_model_execution": False,
        "universal_tensor_ir": False,
        "package_arbitrary_code_execution": False,
        "flux2_production_image_backend": False,
    },
    "limitations": [
        "post-R1 exact-source Qwen CUDA replay unavailable",
        "final-source NVIDIA memory/thermal/power qualification unavailable",
        "AIR-owned FLUX text-encoder/denoiser/VAE execution not implemented",
        "FLUX image parity not qualified",
        "final interactive human Control Room usability not qualified",
    ],
}
(out / "release-manifest.json").write_text(
    json.dumps(manifest, indent=2, sort_keys=True) + "\n",
    encoding="utf-8",
)
PY

(
    cd "$OUT"
    sha256sum         AIR-0.11.0-source.tar.gz         AIR-0.11.0-hosted-qualification-evidence.tar.gz         RELEASE-NOTES.md         RELEASE-PROVENANCE.md         SUPPORT-MATRIX.md         PUBLIC-CONTRACTS.md         release-manifest.json
) > "$OUT/RELEASE-SHA256SUMS.txt"

[[ -z "$(git status --porcelain)" ]] || {
    echo "ERROR: qualification mutated source worktree" >&2
    git status --short >&2
    exit 7
}
[[ "$(git rev-parse HEAD)" == "$HEAD" ]] || {
    echo "ERROR: HEAD changed during qualification" >&2
    exit 7
}

cat > "$OUT/TAG-COMMAND.txt" <<EOF
git tag -a v0.11.0 $HEAD -m "AIR 0.11.0 — Adaptive Execution Foundation"
git push origin v0.11.0
EOF

echo
echo "===== AIR 0.11.0 HOSTED RELEASE CANDIDATE ====="
cat "$OUT/release-identity.txt"
echo
cat "$OUT/version-surfaces.txt"
echo
cat "$OUT/RELEASE-SHA256SUMS.txt"
echo
echo "FINAL_HOSTED_RELEASE_CANDIDATE=PASS"
echo "READY_TO_TAG_WITH_LIMITATIONS=v0.11.0"
echo "GPU_FINAL_SOURCE_QUALIFIED=false"
echo "FLUX2_DEVICE_EXECUTION_QUALIFIED=false"
echo "HUMAN_USABILITY_QUALIFIED=false"
echo "Evidence directory: $OUT"
