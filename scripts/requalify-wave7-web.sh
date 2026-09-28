#!/usr/bin/env bash
set -euo pipefail

EVIDENCE="${1:-}"
MODEL="${2:-}"
PORT="${3:-18360}"

if [[ -z "$EVIDENCE" || ! -d "$EVIDENCE" ||
      -z "$MODEL" || ! -f "$MODEL" ]]; then
    echo "Usage: $0 /path/to/AIR-Modular-Runtime-W7-... /path/to/model.gguf [port]" >&2
    exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PREFIX="$EVIDENCE/prefix"
RC="$EVIDENCE/rc"
MANIFEST="$RC/qualification-manifest.json"
IDENTITY="$EVIDENCE/identity.txt"

for required in     "$PREFIX/bin/air-server"     "$PREFIX/share/air/web/index.html"     "$MANIFEST"     "$IDENTITY"; do
    if [[ ! -e "$required" ]]; then
        echo "ERROR: missing prior Wave 7 artifact: $required" >&2
        exit 2
    fi
done

BASE_HEAD="$(sed -n 's/^head=//p' "$IDENTITY" | head -n1)"
CURRENT_HEAD="$(git rev-parse HEAD)"
BRANCH="$(git branch --show-current)"

if [[ -z "$BASE_HEAD" ]]; then
    echo "ERROR: prior Wave 7 identity does not contain a head." >&2
    exit 2
fi
if [[ "$BRANCH" != "architecture/modular-runtime-r0" ]]; then
    echo "ERROR: expected architecture/modular-runtime-r0; current=$BRANCH" >&2
    exit 2
fi
if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: targeted requalification requires a clean worktree." >&2
    git status --short >&2
    exit 2
fi

ALLOWED=(
    "docs/modular-runtime/WAVE7-QUALIFICATION.md"
    "scripts/air-web-doctor.sh"
    "scripts/requalify-wave7-web.sh"
)

mapfile -t CHANGED < <(git diff --name-only "$BASE_HEAD..$CURRENT_HEAD")
for path in "${CHANGED[@]}"; do
    ok=0
    for allowed in "${ALLOWED[@]}"; do
        if [[ "$path" == "$allowed" ]]; then
            ok=1
            break
        fi
    done
    if [[ "$ok" -ne 1 ]]; then
        echo "ERROR: product-affecting file changed since full Wave 7 qualification: $path" >&2
        echo "Run the complete Wave 7 qualifier instead of targeted web requalification." >&2
        exit 3
    fi
done

if curl -fsS --max-time 1 "http://127.0.0.1:$PORT/health" >/dev/null 2>&1; then
    echo "ERROR: port $PORT already has an AIR-like service. Stop it before requalification." >&2
    exit 2
fi

SERVER_PID=""
cleanup() {
    if [[ -n "$SERVER_PID" ]]; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}
trap cleanup EXIT INT TERM

"$PREFIX/bin/air-server"     -m "$MODEL"     --backend auto     --device 0     --host 127.0.0.1     --port "$PORT"     --manifest "$MANIFEST"     --require-manifest     --prefix-cache 0     --max-active 4     --max-queued 16     --token-budget 64     --prefill-quantum 16     --stream-queue 64     --workers 8     --max-connections 16     --io-timeout 5     --event-log "$RC/web-requal-events.jsonl"     > "$RC/web-requal-server.txt" 2>&1 &
SERVER_PID=$!

ready=0
for _ in $(seq 1 240); do
    if curl -fsS "http://127.0.0.1:$PORT/health" > "$RC/web-requal-health.json" 2>/dev/null; then
        ready=1
        break
    fi
    sleep 0.1
done
if [[ "$ready" -ne 1 ]]; then
    echo "ERROR: isolated AIR server did not become ready." >&2
    exit 1
fi

set +e
AIR_ROOT="$ROOT" AIR_PREFIX="$PREFIX" AIR_URL="http://127.0.0.1:$PORT" bash "$ROOT/scripts/air-web-doctor.sh" 2>&1 |     tee "$RC/web-doctor-requal.txt"
DOCTOR_RC="${PIPESTATUS[0]}"
set -e

kill -TERM "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID"
SERVER_RC="$?"
SERVER_PID=""

{
    echo "base_head=$BASE_HEAD"
    echo "current_head=$CURRENT_HEAD"
    echo "model=$MODEL"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
    echo "web_doctor_requal=$DOCTOR_RC"
    echo "server_shutdown=$SERVER_RC"
    echo "changed_files_since_full_qualification:"
    printf '%s\n' "${CHANGED[@]}"
} > "$RC/web-requal-gates.txt"

cat "$RC/web-requal-gates.txt"

if [[ "$DOCTOR_RC" -ne 0 || "$SERVER_RC" -ne 0 ]]; then
    exit 1
fi

echo
echo "TARGETED WAVE 7 WEB REQUALIFICATION: PASS"
