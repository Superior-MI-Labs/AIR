#!/usr/bin/env bash

MODEL="${1:-}"
PORT="${2:-18360}"
if [ -z "$MODEL" ] || [ ! -f "$MODEL" ]; then
    echo "Usage: $0 /path/to/model.gguf [port]"
    exit 2
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${AIR_PREFIX:-$HOME/.local}"
BIN="${AIR_BIN_DIR:-$PREFIX/bin}"
export PATH="$BIN:$PATH"

EXPECTED_MANIFEST_SCHEMA="$(sed -nE 's/.*execution_manifest_schema_version = ([0-9]+)U.*/\1/p' "$ROOT/include/air/manifest.hpp" | head -n1)"
EXPECTED_BENCHMARK_SCHEMA="$(sed -nE 's/.*benchmark_report_schema = "([^"]+)".*/\1/p' "$ROOT/include/air/benchmark.hpp" | head -n1)"
if [ -z "$EXPECTED_MANIFEST_SCHEMA" ] || [ -z "$EXPECTED_BENCHMARK_SCHEMA" ]; then
    echo "ERROR: failed to derive current evidence schema identities from source" >&2
    exit 2
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${AIR_RC_OUT:-$HOME/Downloads/AIR-RC-validation-$STAMP}"
ARCHIVE="${AIR_RC_ARCHIVE:-$HOME/Downloads/AIR-RC-validation-$STAMP.zip}"
MANIFEST="$OUT/qualification-manifest.json"
mkdir -p "$OUT"
FAIL=0
SERVER_PID=""
DMON_PID=""

cleanup() {
    if [ -n "$DMON_PID" ]; then
        kill "$DMON_PID" 2>/dev/null || true
        wait "$DMON_PID" 2>/dev/null || true
        DMON_PID=""
    fi
    if [ -n "$SERVER_PID" ]; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}
trap cleanup EXIT INT TERM

record() {
    NAME="$1"
    RC="$2"
    echo "$NAME=$RC" >> "$OUT/exit-codes.txt"
    if [ "$RC" -ne 0 ]; then
        FAIL=1
    fi
}

http_status() {
    URL="$1"
    DATA="$2"
    BODY="$3"
    curl -sS -o "$BODY" -w '%{http_code}' \
        -H 'Content-Type: application/json' \
        -d "$DATA" "$URL" 2>>"$OUT/curl-stderr.txt"
}

wait_ready() {
    URL="$1"
    TARGET="$2"
    for _ in $(seq 1 240); do
        if curl -fsS "$URL" > "$TARGET" 2>/dev/null; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

: > "$OUT/exit-codes.txt"
: > "$OUT/curl-stderr.txt"
: > "$OUT/binary-origins.txt"

for TOOL in air-cli air-server air-bench air-qualify air-verify; do
    RESOLVED="$(command -v "$TOOL" 2>/dev/null || true)"
    if [[ "$RESOLVED" == "$BIN/"* ]]; then
        echo "$TOOL=$RESOLVED" >> "$OUT/binary-origins.txt"
    else
        echo "$TOOL=${RESOLVED:-missing}" >> "$OUT/binary-origins.txt"
        record installed_binary_origin 94
    fi
done
if ! grep -q '^installed_binary_origin=' "$OUT/exit-codes.txt"; then
    record installed_binary_origin 0
fi


{
    echo "date: $(date -Is)"
    echo "project_root: $ROOT"
    echo "model: $MODEL"
    echo "model_sha256: $(sha256sum "$MODEL" | awk '{print $1}')"
    echo "model_bytes: $(stat -c %s "$MODEL")"
    echo
    echo "qualification_prefix: $PREFIX"
    echo "expected_manifest_schema: $EXPECTED_MANIFEST_SCHEMA"
    echo "expected_benchmark_schema: $EXPECTED_BENCHMARK_SCHEMA"
    cat "$OUT/binary-origins.txt"
    echo
    for TOOL in air-cli air-server air-bench air-qualify air-verify; do
        "$TOOL" --version 2>&1 || true
    done
    echo
    uname -a
    echo
    cmake --version | head -n 1
    c++ --version | head -n 1
    if command -v nvcc >/dev/null 2>&1; then nvcc --version; fi
    echo
    nvidia-smi
    echo
    air-cli cuda-info
} > "$OUT/system.txt" 2>&1

# Public help/semantic surface. This also makes the evidence archive useful
# when release docs are reviewed later.
{
    air-cli --help
    echo
    air-server --help
    echo
    air-bench --help
    echo
    air-qualify --help
    echo
    air-verify --help
} > "$OUT/help.txt" 2>&1
if grep -q -- '--prefill-chunk' "$OUT/help.txt"; then
    echo "stale --prefill-chunk found" > "$OUT/help-check.txt"
    record help_contract 91
else
    echo "help contract clean" > "$OUT/help-check.txt"
    record help_contract 0
fi

# Real reference/CUDA differential gate. The tolerance is deliberately wider
# than the observed 3080 error while still tight enough to catch broken math.
air-verify -m "$MODEL" \
    --prompt "The capital of France is" \
    --generate 16 --top-k 8 --atol 0.001 --device 0 \
    --output "$OUT/verification.json" \
    > "$OUT/verification.txt" 2>&1
record differential_verification "$?"

# Fresh current-schema qualification. Old manifests are not reused.
air-qualify -m "$MODEL" \
    --prompt "Explain why Lake Superior is important." \
    --workload small --tokens 24 --runs 5 --device 0 --cuda-only --seed 1337 \
    --output "$MANIFEST" > "$OUT/qualification.txt" 2>&1
record qualification "$?"

python3 - "$MANIFEST" "$EXPECTED_MANIFEST_SCHEMA" <<'PY' > "$OUT/manifest-check.txt" 2>&1
import json, sys
p=sys.argv[1]
expected=int(sys.argv[2])
x=json.load(open(p, encoding='utf-8'))
print('schema_version=', x.get('schema_version'), sep='')
print('expected_schema_version=', expected, sep='')
print('air_version=', x.get('air_version'), sep='')
print('manifest_id=', x.get('manifest_id'), sep='')
print('strategy_count=', len(x.get('strategies', [])), sep='')
raise SystemExit(0 if x.get('schema_version') == expected and x.get('strategies') else 1)
PY
record manifest_current "$?"

# The immediately prior schema must fail explicitly rather than being reinterpreted.
LEGACY_SCHEMA=$((EXPECTED_MANIFEST_SCHEMA - 1))
python3 - "$MANIFEST" "$OUT/legacy-manifest.json" "$LEGACY_SCHEMA" <<'PY'
import json,sys
x=json.load(open(sys.argv[1], encoding='utf-8'))
x['schema_version']=int(sys.argv[3])
json.dump(x, open(sys.argv[2], 'w', encoding='utf-8'), separators=(',', ':'))
PY
LEGACY_PORT=$((PORT + 1))
air-server -m "$MODEL" --backend auto --device 0 --host 127.0.0.1 --port "$LEGACY_PORT" \
    --manifest "$OUT/legacy-manifest.json" --require-manifest --prefix-cache 0 \
    > "$OUT/legacy-manifest-server.txt" 2>&1
LEGACY_RC=$?
if [ "$LEGACY_RC" -eq 0 ]; then
    record legacy_manifest_rejected 92
else
    record legacy_manifest_rejected 0
fi

# Installed-package consumer test. This validates the public CMake integration,
# not just the in-tree aliases.
CONSUMER="$OUT/cmake-consumer"
mkdir -p "$CONSUMER"
cat > "$CONSUMER/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.22)
project(AIRConsumer LANGUAGES CXX)
find_package(AIR CONFIG REQUIRED)
add_executable(air_consumer main.cpp)
target_link_libraries(air_consumer PRIVATE AIR::core AIR::cuda)
CMAKE
cat > "$CONSUMER/main.cpp" <<'CPP'
#include <air/version.hpp>
#include <air/types.hpp>
#include <iostream>
int main() {
    std::cout << air::version_string() << " " << sizeof(air::TokenId) << "\n";
    return 0;
}
CPP
cmake -S "$CONSUMER" -B "$CONSUMER/build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PREFIX" \
    > "$OUT/cmake-consumer-configure.txt" 2>&1 && \
cmake --build "$CONSUMER/build" -j2 > "$OUT/cmake-consumer-build.txt" 2>&1 && \
"$CONSUMER/build/air_consumer" > "$OUT/cmake-consumer-run.txt" 2>&1
record cmake_consumer "$?"

# Start the RC server using required current evidence.
nvidia-smi dmon -s pucvmet > "$OUT/gpu-dmon.txt" 2>&1 &
DMON_PID=$!

air-server -m "$MODEL" --backend auto --device 0 --host 127.0.0.1 --port "$PORT" \
    --manifest "$MANIFEST" --require-manifest --prefix-cache 0 \
    --max-active 4 --max-queued 16 --token-budget 64 --prefill-quantum 16 \
    --stream-queue 64 --workers 8 --max-connections 16 --io-timeout 5 \
    --event-log "$OUT/server-events.jsonl" > "$OUT/server.txt" 2>&1 &
SERVER_PID=$!

if wait_ready "http://127.0.0.1:$PORT/health" "$OUT/health.json"; then
    record server_ready 0
else
    record server_ready 93
fi

if [ "$FAIL" -eq 0 ]; then
    for endpoint in / /model /runtime /events /metrics /v1/models; do
        SAFE_NAME="$(printf '%s' "$endpoint" | sed 's#^/$#root#; s#^/##; s#/#-#g')"
        CODE="$(curl -sS -o "$OUT/get-$SAFE_NAME.body" -w '%{http_code}' "http://127.0.0.1:$PORT$endpoint" 2>>"$OUT/curl-stderr.txt")"
        echo "$CODE" > "$OUT/get-$SAFE_NAME.status"
        [ "$CODE" = "200" ]; record "get_$SAFE_NAME" "$?"
    done

    AIR_ROOT="$ROOT" AIR_PREFIX="$PREFIX" AIR_URL="http://127.0.0.1:$PORT" \
        "$ROOT/scripts/air-web-doctor.sh" > "$OUT/web-doctor.txt" 2>&1
    record web_doctor "$?"

    AIR_URL="http://127.0.0.1:$PORT" \
        "$ROOT/scripts/air-generation-doctor.sh" > "$OUT/generation-doctor.txt" 2>&1
    record generation_doctor "$?"

    STATUS="$(http_status "http://127.0.0.1:$PORT/decide" \
        '{"input":"Route this request:","candidates":[{"id":"left","text":"Left","model_text":" left"},{"id":"right","text":"Right","model_text":" right"}],"scoring_policy":"sequence-logprob-mean","output_cardinality":"exactly-one","determinism":"required"}' \
        "$OUT/native-decision.json")"
    echo "$STATUS" > "$OUT/native-decision.status"
    [ "$STATUS" = "200" ]; record native_decision "$?"

    curl -fsS "http://127.0.0.1:$PORT/runtime" > "$OUT/runtime-before.json" 2>/dev/null

    STATUS="$(http_status "http://127.0.0.1:$PORT/generate" \
        '{"prompt":"The capital of France is","max_tokens":8,"temperature":0,"seed":18446744073709551615}' \
        "$OUT/native-generate.json")"
    echo "$STATUS" > "$OUT/native-generate.status"
    [ "$STATUS" = "200" ]; record native_generate "$?"

    STATUS="$(http_status "http://127.0.0.1:$PORT/v1/completions" \
        '{"model":"air","prompt":"The capital of France is","max_tokens":8,"temperature":0,"n":1,"seed":18446744073709551615}' \
        "$OUT/openai-completions.json")"
    echo "$STATUS" > "$OUT/openai-completions.status"
    [ "$STATUS" = "200" ]; record openai_completions "$?"

    STATUS="$(http_status "http://127.0.0.1:$PORT/v1/chat/completions" \
        '{"model":"air","messages":[{"role":"user","content":"Name one Great Lake."}],"max_completion_tokens":8,"temperature":0,"n":1}' \
        "$OUT/openai-chat.json")"
    echo "$STATUS" > "$OUT/openai-chat.status"
    [ "$STATUS" = "200" ]; record openai_chat "$?"

    STATUS="$(http_status "http://127.0.0.1:$PORT/v1/completions" \
        '{"prompt":"test","max_tokens":1,"frequency_penalty":0.1}' \
        "$OUT/unsupported-field.json")"
    echo "$STATUS" > "$OUT/unsupported-field.status"
    [ "$STATUS" = "501" ]; record unsupported_field_rejected "$?"

    STATUS="$(http_status "http://127.0.0.1:$PORT/v1/completions" \
        '{"prompt":"test","max_tokens":1,"max_completion_tokens":1}' \
        "$OUT/conflicting-token-fields.json")"
    echo "$STATUS" > "$OUT/conflicting-token-fields.status"
    [ "$STATUS" = "400" ]; record conflicting_fields_rejected "$?"

    python3 "$ROOT/scripts/stress-server.py" --url "http://127.0.0.1:$PORT" \
        --requests 24 --concurrency 6 --disconnects 2 \
        --prompt "Give one short fact about Lake Superior. " \
        > "$OUT/stress.json" 2> "$OUT/stress-stderr.txt"
    record light_stress "$?"

    curl -fsS "http://127.0.0.1:$PORT/runtime" > "$OUT/runtime-after.json" 2>/dev/null
    curl -fsS "http://127.0.0.1:$PORT/metrics" > "$OUT/metrics-after.txt" 2>/dev/null || true

    python3 - "$OUT/runtime-after.json" <<'PY' > "$OUT/resource-check.txt" 2>&1
import json,sys
x=json.load(open(sys.argv[1], encoding='utf-8'))
c=x.get('capabilities') or {}
checks={
    'queued_requests': x.get('queued_requests') == 0,
    'active_requests': x.get('active_requests') == 0,
    'current_kv_bytes': x.get('current_kv_bytes') == 0,
    'admission_reserved_bytes': c.get('admission_reserved_bytes') == 0,
    'kv_pool_all_free': c.get('kv_pool_allocated_bytes') == c.get('kv_pool_free_bytes'),
    'failed_requests': x.get('failed_requests') == 0,
}
for k,v in checks.items(): print(f'{k}={v}')
print('cancelled_requests=', x.get('cancelled_requests'), sep='')
print('stream_delivery_failures=', x.get('stream_delivery_failures'), sep='')
raise SystemExit(0 if all(checks.values()) else 1)
PY
    record resource_reclamation "$?"
fi

if [ -n "$SERVER_PID" ]; then
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null
    SERVER_RC=$?
    SERVER_PID=""
    if [ "$SERVER_RC" -eq 0 ]; then record server_shutdown 0; else record server_shutdown "$SERVER_RC"; fi
fi
if [ -n "$DMON_PID" ]; then
    kill "$DMON_PID" 2>/dev/null || true
    wait "$DMON_PID" 2>/dev/null || true
    DMON_PID=""
fi

# Deterministic bounded-overload proof on a fresh server instance. A one-active,
# one-queued scheduler must reject at least one synchronized request with HTTP 503.
OVERLOAD_PORT=$((PORT + 2))
air-server -m "$MODEL" --backend auto --device 0 --host 127.0.0.1 --port "$OVERLOAD_PORT" \
    --manifest "$MANIFEST" --require-manifest --prefix-cache 0 \
    --max-active 1 --max-queued 1 --token-budget 1 --prefill-quantum 1 \
    --stream-queue 16 --workers 8 --max-connections 16 --io-timeout 5 \
    --event-log "$OUT/overload-server-events.jsonl" > "$OUT/overload-server.txt" 2>&1 &
SERVER_PID=$!

if wait_ready "http://127.0.0.1:$OVERLOAD_PORT/health" "$OUT/overload-health.json"; then
    record overload_server_ready 0
    python3 "$ROOT/scripts/overload-server.py" \
        --url "http://127.0.0.1:$OVERLOAD_PORT" \
        --requests 12 --tokens 256 \
        > "$OUT/overload.json" 2> "$OUT/overload-stderr.txt"
    record bounded_overload "$?"

    curl -fsS "http://127.0.0.1:$OVERLOAD_PORT/runtime" \
        > "$OUT/overload-runtime-after.json" 2>/dev/null
    python3 - "$OUT/overload-runtime-after.json" <<'PY' \
        > "$OUT/overload-resource-check.txt" 2>&1
import json,sys
x=json.load(open(sys.argv[1], encoding='utf-8'))
c=x.get('capabilities') or {}
checks={
    'queued_requests': x.get('queued_requests') == 0,
    'active_requests': x.get('active_requests') == 0,
    'current_kv_bytes': x.get('current_kv_bytes') == 0,
    'admission_reserved_bytes': c.get('admission_reserved_bytes') == 0,
    'kv_pool_all_free': c.get('kv_pool_allocated_bytes') == c.get('kv_pool_free_bytes'),
    'rejected_overload_requests': (x.get('rejected_overload_requests') or 0) >= 1,
}
for k,v in checks.items(): print(f'{k}={v}')
raise SystemExit(0 if all(checks.values()) else 1)
PY
    record overload_resource_reclamation "$?"
else
    record overload_server_ready 95
fi

if [ -n "$SERVER_PID" ]; then
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null
    OVERLOAD_SERVER_RC=$?
    SERVER_PID=""
    if [ "$OVERLOAD_SERVER_RC" -eq 0 ]; then
        record overload_server_shutdown 0
    else
        record overload_server_shutdown "$OVERLOAD_SERVER_RC"
    fi
fi

python3 - "$OUT" <<'PY' > "$OUT/summary.txt"
import json,pathlib,statistics,sys
root=pathlib.Path(sys.argv[1])
print('AIR release-candidate validation summary')
for name in ('verification.json','qualification-manifest.json','runtime-after.json','stress.json','overload.json'):
    p=root/name
    if p.exists():
        try:
            x=json.loads(p.read_text())
        except Exception:
            continue
        if name=='verification.json':
            print('verification_schema=',x.get('schema'),sep='')
            print('verification_positions=',len(x.get('decisions',[])),sep='')
            print('verification_all_top1_match=',all(d.get('top1_match',False) for d in x.get('decisions',[])),sep='')
        elif name=='qualification-manifest.json':
            print('manifest_schema=',x.get('schema_version'),sep='')
            print('manifest_strategies=',len(x.get('strategies',[])),sep='')
        elif name=='runtime-after.json':
            c=x.get('capabilities') or {}
            print('completed_requests=',x.get('completed_requests'),sep='')
            print('failed_requests=',x.get('failed_requests'),sep='')
            print('cancelled_requests=',x.get('cancelled_requests'),sep='')
            print('current_kv_bytes=',x.get('current_kv_bytes'),sep='')
            print('admission_reserved_bytes=',c.get('admission_reserved_bytes'),sep='')
            print('kv_pool_allocated_bytes=',c.get('kv_pool_allocated_bytes'),sep='')
            print('kv_pool_free_bytes=',c.get('kv_pool_free_bytes'),sep='')
        elif name=='stress.json':
            print('stress_successes=',x.get('successes'),'/',x.get('requests'),sep='')
            print('stress_errors=',len(x.get('errors',[])),sep='')
        elif name=='overload.json':
            print('overload_http_200=',x.get('http_200'),sep='')
            print('overload_http_503=',x.get('http_503'),sep='')
            print('overload_rejected_requests=',x.get('rejected_overload_requests'),sep='')

dmon=root/'gpu-dmon.txt'
if dmon.exists():
    temps=[]; powers=[]; clocks=[]; pviol=[]; tviol=[]
    for line in dmon.read_text(errors='ignore').splitlines():
        if not line.strip() or line.lstrip().startswith('#'): continue
        a=line.split()
        try:
            powers.append(float(a[1])); temps.append(float(a[2])); clocks.append(float(a[11])); pviol.append(float(a[12])); tviol.append(int(a[13]))
        except Exception: pass
    if temps:
        print('gpu_temp_mean_C=',round(statistics.mean(temps),2),sep='')
        print('gpu_temp_max_C=',max(temps),sep='')
    if powers: print('gpu_power_max_W=',max(powers),sep='')
    if clocks: print('gpu_pclk_mean_MHz=',round(statistics.mean(clocks),2),sep='')
    if pviol: print('gpu_power_violation_samples=',sum(1 for x in pviol if x>0),sep='')
    if tviol: print('gpu_thermal_violation_samples=',sum(1 for x in tviol if x!=0),sep='')
PY

{
    echo "overall_fail=$FAIL"
    cat "$OUT/exit-codes.txt"
} > "$OUT/gates.txt"

rm -f "$ARCHIVE"
(cd "$HOME/Downloads" && zip -qr "$ARCHIVE" "$(basename "$OUT")")
ZIP_RC=$?

printf '\n===== AIR RC VALIDATION =====\n'
cat "$OUT/summary.txt" 2>/dev/null
printf '\n'
cat "$OUT/gates.txt"
printf '\nUpload archive:\n%s\n' "$ARCHIVE"

if [ "$ZIP_RC" -ne 0 ]; then exit "$ZIP_RC"; fi
exit "$FAIL"
