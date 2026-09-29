#!/usr/bin/env bash
set -euo pipefail

MODEL="${1:-}"
PORT="${2:-18360}"
LLAMA_ENDPOINT="${MEF_LLAMA_ENDPOINT:-http://127.0.0.1:1920}"

if [[ -z "$MODEL" || ! -f "$MODEL" ]]; then
    echo "Usage: $0 /path/to/model.gguf [AIR qualification port]" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

EXPECTED_VERSION="0.10.0"
BRANCH="$(git branch --show-current)"
HEAD="$(git rev-parse HEAD)"
TREE="$(git rev-parse HEAD^{tree})"

if [[ "$BRANCH" != "architecture/modular-runtime-r0" ]]; then
    echo "ERROR: final 0.10.0 qualification requires architecture/modular-runtime-r0; current=$BRANCH" >&2
    exit 2
fi
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: final release qualification requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

VERSION="$(sed -nE 's/^project\(AIR VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt | head -n1)"
if [[ "$VERSION" != "$EXPECTED_VERSION" ]]; then
    echo "ERROR: CMake project version is '$VERSION', expected '$EXPECTED_VERSION'." >&2
    exit 2
fi

for doc in     docs/RELEASE-0.10.0.md     docs/RELEASE-PROVENANCE.md     docs/SUPPORT_MATRIX.md     docs/PUBLIC_CONTRACTS.md     docs/ARCHITECTURE.md     docs/modular-runtime/WAVE7-QUALIFICATION.md     docs/modular-runtime/WAVE8-RELEASE.md; do
    [[ -f "$doc" ]] || { echo "ERROR: missing release document: $doc" >&2; exit 2; }
done

require_text() {
    local needle="$1"
    local file="$2"
    python3 - "$needle" "$file" <<'PY'
import pathlib
import re
import sys

needle = " ".join(sys.argv[1].split())
text = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8")
normalized = re.sub(r"\s+", " ", text)
if needle not in normalized:
    print(f"ERROR: required release text missing from {sys.argv[2]}:", file=sys.stderr)
    print(f"  {sys.argv[1]}", file=sys.stderr)
    raise SystemExit(2)
PY
}

require_text 'AIR 0.10.0' README.md
require_text 'Qwen2 remains the only qualified production model architecture in AIR 0.10.0.' README.md
require_text 'schema_version = 10' docs/PUBLIC_CONTRACTS.md
require_text 'air.benchmark.v11' docs/PUBLIC_CONTRACTS.md
require_text 'AIR 0.10.0 production model support remains intentionally narrow:' docs/RELEASE-0.10.0.md
require_text '`qwen2` architecture metadata;' docs/RELEASE-0.10.0.md
require_text 'additional qualified production model families.' docs/RELEASE-0.10.0.md

echo "Release preflight: version/docs/source-boundary checks PASS"

# Final source-level architecture boundary: executors must not recover canonical
# Qwen2/GGUF tensor names after preparation.
if grep -En     'blk\.|token_embd\.weight|output_norm\.weight|attn_q\.weight|attn_q\.bias|find_tensor\('     src/reference/reference_executor.cpp src/cuda/cuda_backend.cu; then
    echo "ERROR: executor source-name dependency reappeared after Wave 7." >&2
    exit 3
fi

if ! curl -fsS --max-time 2 "$LLAMA_ENDPOINT/health" >/dev/null 2>&1; then
    echo "ERROR: frozen MEF R0 requires the qualified llama.cpp provider at $LLAMA_ENDPOINT" >&2
    echo "Start it before running final release qualification." >&2
    exit 2
fi

if git rev-parse -q --verify refs/tags/v0.10.0 >/dev/null 2>&1; then
    TAG_HEAD="$(git rev-list -n1 v0.10.0)"
    if [[ "$TAG_HEAD" != "$HEAD" ]]; then
        echo "ERROR: v0.10.0 already exists and does not point to current HEAD." >&2
        exit 4
    fi
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_RELEASE_OUT:-$HOME/Downloads/AIR-0.10.0-RC-$STAMP}"
AIR_OUT="$OUT/air"
MEF_OUT="$OUT/mef"
mkdir -p "$OUT"

{
    echo "date=$(date -Is)"
    echo "version=$VERSION"
    echo "branch=$BRANCH"
    echo "head=$HEAD"
    echo "tree=$TREE"
    echo "model=$MODEL"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
    echo "llama_endpoint=$LLAMA_ENDPOINT"
} > "$OUT/release-identity.txt"

echo "=== AIR 0.10.0 FINAL LOCAL QUALIFICATION ==="
AIR_W7_OUT="$AIR_OUT" bash "$ROOT/scripts/qualify-modular-runtime-r0.sh" "$MODEL" "$PORT"

PREFIX="$AIR_OUT/prefix"
for tool in air-cli air-server air-bench air-qualify; do
    OUTPUT="$("$PREFIX/bin/$tool" --version 2>&1)"
    echo "$tool=$OUTPUT" >> "$OUT/version-surfaces.txt"
    if ! grep -Fq "$EXPECTED_VERSION" <<<"$OUTPUT"; then
        echo "ERROR: $tool does not report AIR $EXPECTED_VERSION" >&2
        exit 5
    fi
done

VERIFY_HELP="$("$PREFIX/bin/air-verify" --help 2>&1)"
printf 'air-verify=%s\n' "$(head -n1 <<<"$VERIFY_HELP")" >> "$OUT/version-surfaces.txt"
if ! grep -Fq "$EXPECTED_VERSION" <<<"$VERIFY_HELP"; then
    echo "ERROR: air-verify help does not report AIR $EXPECTED_VERSION" >&2
    exit 5
fi

CMAKE_VERSION_FILE="$PREFIX/lib/cmake/AIR/AIRConfigVersion.cmake"
if ! grep -Fq 'set(PACKAGE_VERSION "0.10.0")' "$CMAKE_VERSION_FILE"; then
    echo "ERROR: installed CMake package does not report 0.10.0" >&2
    exit 5
fi
echo "cmake_package_version=0.10.0" >> "$OUT/version-surfaces.txt"

echo
echo "=== AIR 0.10.0 FINAL FROZEN MEF R0 QUALIFICATION ==="
AIR_MEF_OUT="$MEF_OUT" MEF_LLAMA_ENDPOINT="$LLAMA_ENDPOINT" bash "$ROOT/scripts/qualify-wave7-mef-r0.sh" "$AIR_OUT" "$MODEL"

MEF_ARCHIVE="$(find "$MEF_OUT" -maxdepth 1 -type f     -name 'Superior-MI-MEF-R0-Evidence-*.zip'     -printf '%T@ %p\n' | sort -nr | head -n1 | cut -d' ' -f2-)"
if [[ -z "$MEF_ARCHIVE" || ! -f "$MEF_ARCHIVE" ]]; then
    echo "ERROR: final MEF evidence archive not found." >&2
    exit 6
fi

SOURCE_ARCHIVE="$OUT/AIR-0.10.0-source.tar.gz"
git archive --format=tar --prefix="AIR-0.10.0/" "$HEAD" | gzip -n -9 > "$SOURCE_ARCHIVE"

cp docs/RELEASE-0.10.0.md "$OUT/RELEASE-NOTES.md"
cp docs/RELEASE-PROVENANCE.md "$OUT/RELEASE-PROVENANCE.md"
cp docs/SUPPORT_MATRIX.md "$OUT/SUPPORT-MATRIX.md"

AIR_RC_ARCHIVE="$AIR_OUT/rc.zip"
if [[ ! -f "$AIR_RC_ARCHIVE" ]]; then
    echo "ERROR: AIR RC evidence archive not found: $AIR_RC_ARCHIVE" >&2
    exit 6
fi

(
    cd "$OUT"
    sha256sum         "$(basename "$SOURCE_ARCHIVE")"         "air/rc.zip"         "mef/$(basename "$MEF_ARCHIVE")"
) > "$OUT/RELEASE-SHA256SUMS.txt"

python3 - "$OUT" "$HEAD" "$TREE" "$VERSION" "$MODEL" "$SOURCE_ARCHIVE" "$AIR_RC_ARCHIVE" "$MEF_ARCHIVE" <<'PY'
import hashlib
import json
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
head, tree, version = sys.argv[2:5]
model = pathlib.Path(sys.argv[5])
source = pathlib.Path(sys.argv[6])
air_rc = pathlib.Path(sys.argv[7])
mef = pathlib.Path(sys.argv[8])

def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

manifest = {
    "schema": "air.release.v1",
    "version": version,
    "git_head": head,
    "git_tree": tree,
    "qualified_model": {
        "path": str(model),
        "sha256": sha(model),
    },
    "artifacts": {
        "source": {"name": source.name, "sha256": sha(source)},
        "air_qualification": {"name": air_rc.name, "sha256": sha(air_rc)},
        "mef_r0_evidence": {"name": mef.name, "sha256": sha(mef)},
    },
    "claims": {
        "qualified_architecture": "qwen2",
        "universal_gguf": False,
        "universal_neural_model_ir": False,
        "builder_to_air_arbitrary_model_compilation": False,
    },
}
(out / "release-manifest.json").write_text(
    json.dumps(manifest, indent=2, sort_keys=True) + "\n"
)
PY

# Qualification must not modify the source tree.
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: final qualification mutated the source worktree." >&2
    git status --short >&2
    exit 7
fi
if [[ "$(git rev-parse HEAD)" != "$HEAD" ]]; then
    echo "ERROR: source HEAD changed during final qualification." >&2
    exit 7
fi

cat > "$OUT/TAG-COMMAND.txt" <<EOF
git tag -a v0.10.0 $HEAD -m "AIR 0.10.0 — Modular Model Architecture Boundary"
git push origin v0.10.0
EOF

echo
echo "===== AIR 0.10.0 FINAL RELEASE CANDIDATE ====="
cat "$OUT/release-identity.txt"
echo
cat "$OUT/version-surfaces.txt"
echo
cat "$OUT/RELEASE-SHA256SUMS.txt"
echo
echo "FINAL_RELEASE_CANDIDATE=PASS"
echo "READY_TO_TAG=v0.10.0"
echo "Evidence directory: $OUT"
