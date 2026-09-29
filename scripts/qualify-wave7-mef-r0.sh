#!/usr/bin/env bash
set -euo pipefail

AIR_EVIDENCE="${1:-}"
AIR_MODEL_FILE="${2:-}"
MEF_REPO="${3:-$HOME/Projects/Superior-MI-Model-Execution-Fabric}"
AIR_PORT="${AIR_MEF_PORT:-8181}"
LLAMA_ENDPOINT="${MEF_LLAMA_ENDPOINT:-http://127.0.0.1:1920}"

if [[ -z "$AIR_EVIDENCE" || ! -d "$AIR_EVIDENCE" ||
      -z "$AIR_MODEL_FILE" || ! -f "$AIR_MODEL_FILE" ]]; then
    echo "Usage: $0 /path/to/AIR-Modular-Runtime-W7-... /path/to/model.gguf [MEF repo]" >&2
    exit 2
fi
if [[ ! -d "$MEF_REPO/.git" ]]; then
    echo "ERROR: MEF repository not found: $MEF_REPO" >&2
    exit 2
fi

AIR_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$AIR_ROOT"

AIR_PREFIX="$AIR_EVIDENCE/prefix"
AIR_RC="$AIR_EVIDENCE/rc"
AIR_SERVER="$AIR_PREFIX/bin/air-server"
AIR_MANIFEST="$AIR_RC/qualification-manifest.json"
AIR_IDENTITY="$AIR_EVIDENCE/identity.txt"
AIR_ENDPOINT="http://127.0.0.1:$AIR_PORT"

for required in "$AIR_SERVER" "$AIR_MANIFEST" "$AIR_IDENTITY"; do
    if [[ ! -e "$required" ]]; then
        echo "ERROR: missing qualified AIR artifact: $required" >&2
        exit 2
    fi
done

QUALIFIED_HEAD="$(sed -n 's/^head=//p' "$AIR_IDENTITY" | head -n1)"
CURRENT_HEAD="$(git rev-parse HEAD)"
CURRENT_BRANCH="$(git branch --show-current)"
if [[ -z "$QUALIFIED_HEAD" ]]; then
    echo "ERROR: qualified AIR evidence contains no source head." >&2
    exit 2
fi
if [[ "$CURRENT_BRANCH" != "architecture/modular-runtime-r0" ]]; then
    echo "ERROR: expected architecture/modular-runtime-r0; current=$CURRENT_BRANCH" >&2
    exit 2
fi
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: MEF gate requires a clean AIR worktree." >&2
    git status --short >&2
    exit 2
fi

# Reuse is valid only when changes since the full AIR product qualification are
# qualification/docs-only. Any product-affecting delta requires a full Wave 7 rerun.
ALLOWED_PREFIXES=(
    "docs/modular-runtime/WAVE7-QUALIFICATION.md"
    "scripts/air-web-doctor.sh"
    "scripts/requalify-wave7-web.sh"
    "scripts/qualify-wave7-mef-r0.sh"
)
mapfile -t AIR_CHANGED < <(git diff --name-only "$QUALIFIED_HEAD..$CURRENT_HEAD")
for path in "${AIR_CHANGED[@]}"; do
    allowed=0
    for candidate in "${ALLOWED_PREFIXES[@]}"; do
        if [[ "$path" == "$candidate" ]]; then
            allowed=1
            break
        fi
    done
    if [[ "$allowed" -ne 1 ]]; then
        echo "ERROR: product-affecting AIR file changed after full Wave 7 qualification: $path" >&2
        echo "Run the complete Wave 7 qualifier before MEF requalification." >&2
        exit 3
    fi
done

if ! curl -fsS --max-time 2 "$LLAMA_ENDPOINT/v1/models" > /tmp/mef-r0-llama-models.json 2>/dev/null; then
    echo "ERROR: qualified llama.cpp endpoint is not reachable at $LLAMA_ENDPOINT" >&2
    echo "Start the R0 llama.cpp provider, then rerun this script." >&2
    exit 2
fi

LLAMA_MODEL="$(
python3 - /tmp/mef-r0-llama-models.json <<'PY'
import json,sys
x=json.load(open(sys.argv[1], encoding="utf-8"))
ids=[m.get("id","") for m in x.get("data",[]) if m.get("id")]
if not ids:
    raise SystemExit("llama.cpp advertised no model id")
print(ids[0])
PY
)"

if curl -fsS --max-time 1 "$AIR_ENDPOINT/health" >/dev/null 2>&1; then
    echo "ERROR: something is already serving AIR at $AIR_ENDPOINT" >&2
    echo "Stop it so this gate can use the exact isolated Wave 7 binary." >&2
    exit 2
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_MEF_OUT:-$HOME/Downloads/AIR-MEF-R0-Requal-$STAMP}"
MEF_WT="$OUT/mef-r0-qualified"
mkdir -p "$OUT"

AIR_PID=""
cleanup() {
    if [[ -n "$AIR_PID" ]] && kill -0 "$AIR_PID" 2>/dev/null; then
        kill -TERM "$AIR_PID" 2>/dev/null || true
        wait "$AIR_PID" 2>/dev/null || true
    fi
    git -C "$MEF_REPO" worktree remove --force "$MEF_WT" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

"$AIR_SERVER"     -m "$AIR_MODEL_FILE"     --backend auto     --device 0     --host 127.0.0.1     --port "$AIR_PORT"     --manifest "$AIR_MANIFEST"     --require-manifest     --prefix-cache 0     --max-active 4     --max-queued 16     --token-budget 64     --prefill-quantum 16     --stream-queue 64     --workers 8     --max-connections 16     --io-timeout 5     --event-log "$OUT/air-events.jsonl"     > "$OUT/air-server.txt" 2>&1 &
AIR_PID=$!

READY=0
for _ in $(seq 1 240); do
    if curl -fsS "$AIR_ENDPOINT/health" > "$OUT/air-health.json" 2>/dev/null; then
        READY=1
        break
    fi
    sleep 0.1
done
if [[ "$READY" -ne 1 ]]; then
    echo "ERROR: isolated AIR server did not become ready." >&2
    exit 1
fi

curl -fsS "$AIR_ENDPOINT/v1/models" > "$OUT/air-models.json"
AIR_MODEL="$(
python3 - "$OUT/air-models.json" <<'PY'
import json,sys
x=json.load(open(sys.argv[1], encoding="utf-8"))
ids=[m.get("id","") for m in x.get("data",[]) if m.get("id")]
if not ids:
    raise SystemExit("AIR advertised no model id")
print(ids[0])
PY
)"

cd "$MEF_REPO"
git fetch --tags origin >/dev/null
git worktree add --detach "$MEF_WT" mef-r0-qualified >/dev/null

MEF_HEAD="$(git -C "$MEF_WT" rev-parse HEAD)"
if [[ "$MEF_HEAD" != "33f63246244f91acfba5659bba80447e6e181108" ]]; then
    echo "ERROR: frozen MEF tag resolved to unexpected commit: $MEF_HEAD" >&2
    exit 4
fi

{
    echo "date=$(date -Is)"
    echo "air_qualified_head=$QUALIFIED_HEAD"
    echo "air_current_head=$CURRENT_HEAD"
    echo "air_model_file=$AIR_MODEL_FILE"
    echo "air_model_sha256=$(sha256sum "$AIR_MODEL_FILE" | awk '{print $1}')"
    echo "air_provider_model=$AIR_MODEL"
    echo "air_endpoint=$AIR_ENDPOINT"
    echo "llama_endpoint=$LLAMA_ENDPOINT"
    echo "llama_provider_model=$LLAMA_MODEL"
    echo "mef_tag=mef-r0-qualified"
    echo "mef_head=$MEF_HEAD"
    echo "air_changed_since_product_qualification:"
    printf '%s\n' "${AIR_CHANGED[@]}"
} > "$OUT/identity.txt"

cd "$MEF_WT"

MEF_LLAMA_ENDPOINT="$LLAMA_ENDPOINT" MEF_AIR_ENDPOINT="$AIR_ENDPOINT" MEF_MODEL_ROOT="$HOME/Models" MEF_LLAMA_MODEL="$LLAMA_MODEL" MEF_AIR_MODEL="$AIR_MODEL" bash scripts/qualify-r0.sh 2>&1 | tee "$OUT/mef-r0-qualification.txt"

bash scripts/collect-r0-evidence.sh 2>&1 | tee "$OUT/mef-r0-package.txt"

ARCHIVE="$(find "$MEF_WT" -maxdepth 1 -type f     -name 'Superior-MI-MEF-R0-Evidence-*.zip'     -printf '%T@ %p\n' | sort -nr | head -n1 | cut -d' ' -f2-)"

if [[ -z "$ARCHIVE" || ! -f "$ARCHIVE" ]]; then
    echo "ERROR: MEF R0 evidence archive was not produced." >&2
    exit 1
fi

FINAL_ARCHIVE="$OUT/$(basename "$ARCHIVE")"
cp "$ARCHIVE" "$FINAL_ARCHIVE"
sha256sum "$FINAL_ARCHIVE" > "$OUT/MEF-EVIDENCE-SHA256.txt"

kill -TERM "$AIR_PID" 2>/dev/null || true
wait "$AIR_PID"
AIR_SHUTDOWN_RC="$?"
AIR_PID=""

{
    echo "mef_r0_qualification=0"
    echo "air_shutdown=$AIR_SHUTDOWN_RC"
    echo "evidence_archive=$FINAL_ARCHIVE"
    cat "$OUT/MEF-EVIDENCE-SHA256.txt"
} > "$OUT/gates.txt"

cat "$OUT/gates.txt"
if [[ "$AIR_SHUTDOWN_RC" -ne 0 ]]; then
    exit "$AIR_SHUTDOWN_RC"
fi

echo
echo "WAVE 7 EXTERNAL MEF R0 COMPATIBILITY: PASS"
echo "Evidence directory: $OUT"
