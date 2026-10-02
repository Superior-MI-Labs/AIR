#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build-adaptive-preflight}"
PORT="${2:-18911}"
OUT="${AIR_P11_HOSTED_OUT:-${RUNNER_TEMP:-/tmp}/air-prompt11-hosted}"
PREFIX="$OUT/prefix"
MODEL="$OUT/tiny-qwen2.gguf"
SERVER_LOG="$OUT/server.log"
SERVER_PID=""

cleanup() {
    set +e
    if [[ -n "$SERVER_PID" ]]; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}
trap cleanup EXIT

rm -rf "$OUT"
mkdir -p "$OUT"

[[ -d "$BUILD_DIR" ]] || {
    echo "ERROR: hosted hardening requires existing preflight build: $BUILD_DIR" >&2
    exit 2
}

echo "=== PROMPT 11 HOSTED RC HARDENING ==="
echo "source=$(git -C "$ROOT" rev-parse HEAD)"
echo "build=$BUILD_DIR"
echo "out=$OUT"

echo
echo "=== INSTALL BUILT PACKAGE ==="
cmake --install "$BUILD_DIR" --prefix "$PREFIX"     > "$OUT/install.log" 2>&1

for binary in air-cli air-server air-bench air-qualify air-verify air-strategy-probe; do
    [[ -x "$PREFIX/bin/$binary" ]] || {
        echo "ERROR: installed binary missing: $binary" >&2
        exit 3
    }
done

echo
echo "=== GENERATE TINY QUALIFIED GGUF FIXTURE ==="
python3 "$ROOT/tests/make_tiny_gguf.py" "$MODEL" --context 64     > "$OUT/tiny-model.log"
[[ -s "$MODEL" ]] || {
    echo "ERROR: tiny GGUF fixture was not created" >&2
    exit 3
}

echo
echo "=== EXTERNAL CMAKE CONSUMER ==="
CONSUMER="$OUT/consumer"
mkdir -p "$CONSUMER"
cat > "$CONSUMER/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.22)
project(AIRHostedConsumer LANGUAGES CXX)
find_package(AIR CONFIG REQUIRED)
add_executable(air_hosted_consumer main.cpp)
target_link_libraries(air_hosted_consumer PRIVATE AIR::core AIR::cuda)
CMAKE
cat > "$CONSUMER/main.cpp" <<'CPP'
#include <air/version.hpp>
#include <air/workload.hpp>
#include <iostream>
int main() {
    air::IterativeRequestProfile iterative{4U, 1U};
    air::ExecutionWorkloadProfile workload{iterative};
    std::cout << air::version_string() << " "
              << air::to_string(air::execution_workload_kind(workload))
              << "\n";
    return 0;
}
CPP
cmake -S "$CONSUMER" -B "$CONSUMER/build"     -DCMAKE_BUILD_TYPE=Release     -DCMAKE_PREFIX_PATH="$PREFIX"     > "$OUT/consumer-configure.log" 2>&1
cmake --build "$CONSUMER/build" -j2     > "$OUT/consumer-build.log" 2>&1
"$CONSUMER/build/air_hosted_consumer"     > "$OUT/consumer-run.txt"

wait_health() {
    local attempt
    for attempt in $(seq 1 240); do
        if curl -fsS --max-time 1             "http://127.0.0.1:$PORT/health"             > "$OUT/health.json" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$SERVER_PID" 2>/dev/null; then
            echo "ERROR: air-server exited before becoming healthy" >&2
            tail -n 160 "$SERVER_LOG" >&2 || true
            return 1
        fi
        sleep 0.1
    done
    echo "ERROR: air-server did not become healthy" >&2
    tail -n 160 "$SERVER_LOG" >&2 || true
    return 1
}

start_server() {
    : > "$SERVER_LOG"
    "$PREFIX/bin/air-server"         -m "$MODEL"         --backend reference         --host 127.0.0.1         --port "$PORT"         --no-manifest         --execution-observation detailed         --execution-span-capacity 4096         --max-active 2         --max-queued 8         --token-budget 16         --prefill-quantum 8         --reference-kv-page-tokens 4         --prefix-cache 2         --stream-queue 64         --workers 4         --max-connections 8         --io-timeout 5         --web-root "$PREFIX/share/air/web"         --event-log "$OUT/server-events.jsonl"         > "$SERVER_LOG" 2>&1 &
    SERVER_PID=$!
    wait_health
}

stop_server() {
    if [[ -n "$SERVER_PID" ]]; then
        kill -TERM "$SERVER_PID"
        wait "$SERVER_PID"
        SERVER_PID=""
    fi
}

fetch_json() {
    local path="$1"
    local name="$2"
    local code
    code="$(
        curl -sS --max-time 10             -o "$OUT/$name.json"             -w '%{http_code}'             "http://127.0.0.1:$PORT$path"
    )"
    [[ "$code" == "200" ]] || {
        echo "ERROR: GET $path returned HTTP $code" >&2
        cat "$OUT/$name.json" >&2 || true
        exit 4
    }
    python3 -m json.tool "$OUT/$name.json"         > "$OUT/$name.pretty.json"
}

echo
echo "=== INSTALLED REFERENCE SERVER FIRST START ==="
start_server

for pair in     "/health:health"     "/model:model"     "/runtime:runtime"     "/machine:machine"     "/environment:environment"     "/events:events"     "/timeline:timeline-before"     "/execution-graphs:graphs-before"     "/semantics:semantics"     "/v1/models:models"; do
    fetch_json "${pair%%:*}" "${pair##*:}"
done

ROOT_CODE="$(
    curl -sS --max-time 10         -o "$OUT/root.html"         -w '%{http_code}'         "http://127.0.0.1:$PORT/"
)"
[[ "$ROOT_CODE" == "200" ]]
grep -Fq 'air-web-version" content="3.3.0"' "$OUT/root.html"
grep -Fq '/app/main-v330.js' "$OUT/root.html"

METRICS_CODE="$(
    curl -sS --max-time 10         -o "$OUT/metrics.txt"         -w '%{http_code}'         "http://127.0.0.1:$PORT/metrics"
)"
[[ "$METRICS_CODE" == "200" ]]
grep -Fq 'air_active_requests ' "$OUT/metrics.txt"

AIR_ROOT="$ROOT" AIR_PREFIX="$PREFIX" AIR_URL="http://127.0.0.1:$PORT"     bash "$ROOT/scripts/air-web-doctor.sh"     > "$OUT/web-doctor.txt" 2>&1

echo
echo "=== INSTALLED HTTP GENERATION / DECISION ==="
GEN_CODE="$(
    curl -sS --max-time 30         -o "$OUT/generate.json"         -w '%{http_code}'         -H 'Content-Type: application/json'         -d '{"prompt":"user","max_tokens":4,"temperature":0,"top_p":1,"top_k":0,"seed":7,"stream":false}'         "http://127.0.0.1:$PORT/generate"
)"
[[ "$GEN_CODE" == "200" ]] || {
    echo "ERROR: generation returned HTTP $GEN_CODE" >&2
    cat "$OUT/generate.json" >&2 || true
    exit 5
}

DECIDE_CODE="$(
    curl -sS --max-time 30         -o "$OUT/decide.json"         -w '%{http_code}'         -H 'Content-Type: application/json'         -d '{"input":"a","candidates":[{"id":"a","text":"A","model_text":"a"},{"id":"b","text":"B","model_text":"b"}],"scoring_policy":"sequence-logprob-mean","output_cardinality":"exactly-one","determinism":"required"}'         "http://127.0.0.1:$PORT/decide"
)"
[[ "$DECIDE_CODE" == "200" ]] || {
    echo "ERROR: Decision returned HTTP $DECIDE_CODE" >&2
    cat "$OUT/decide.json" >&2 || true
    exit 5
}

BAD_CODE="$(
    curl -sS --max-time 10         -o "$OUT/invalid-request.json"         -w '%{http_code}'         -H 'Content-Type: application/json'         -d '{"prompt":"a","max_tokens":1,"unsupported_field":true}'         "http://127.0.0.1:$PORT/generate"
)"
[[ "$BAD_CODE" == "400" ]] || {
    echo "ERROR: unsupported request field expected HTTP 400, got $BAD_CODE" >&2
    exit 5
}

fetch_json "/runtime" "runtime-after"
fetch_json "/timeline" "timeline-after"
fetch_json "/execution-graphs" "graphs-after"
fetch_json "/events" "events-after"

python3 - "$OUT" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
load = lambda name: json.loads((root / name).read_text())

health = load("health.json")
model = load("model.json")
runtime = load("runtime-after.json")
machine = load("machine.json")
environment = load("environment.json")
semantics = load("semantics.json")
generation = load("generate.json")
decision = load("decide.json")
timeline = load("timeline-after.json")
graphs = load("graphs-after.json")
events = load("events-after.json")
invalid = load("invalid-request.json")

assert health["status"] == "ok"
assert health["backend"] == "reference"
assert model["architecture"] == "qwen2"
assert model["backend"] == "reference"
assert runtime["backend"] == "reference"

assert machine["fingerprint"]
assert environment["topology_fingerprint"] == machine["fingerprint"]
assert isinstance(machine["nodes"], list) and machine["nodes"]
assert isinstance(environment["resources"], list)

packages = semantics["packages"]
assert len(packages) == 1
assert packages[0]["package_id"] == "flux2-klein.oracle-v1"
assert packages[0]["resolved_count"] == 2
assert packages[0]["missing_count"] == 9

assert generation.get("text") is not None
metrics = generation["metrics"]
assert metrics["backend"] == "reference"
assert metrics["generated_tokens"] > 0

selected = decision["selected_candidate_ids"]
assert len(selected) == 1
assert selected[0] in {"a", "b"}
assert decision["score_semantics"] == "candidate-set-normalized"

assert timeline["level"] == "detailed"
assert timeline["dropped_spans"] == 0
assert isinstance(timeline["spans"], list) and timeline["spans"]

assert graphs["level"] == "detailed"
assert graphs["derivation_failures"] == 0
assert isinstance(graphs["observations"], list) and graphs["observations"]

for observation in graphs["observations"]:
    graph = observation.get("graph")
    if not graph:
        continue
    assert graph["schema_version"] == 3
    assert graph["workload_kind"] == "autoregressive-tokens"
    assert graph["binding"] == "air-executable"

assert isinstance(events, list)
assert any(item.get("type") == "request_complete" for item in events)
assert any(item.get("type") == "decision_complete" for item in events)
assert invalid["error"]["type"]

print("hosted_health_model_runtime=PASS")
print("hosted_machine_environment_authority=PASS")
print("hosted_semantic_missing_surface=PASS")
print("hosted_generation=PASS")
print("hosted_decision=PASS")
print("hosted_invalid_request=PASS")
print("hosted_detailed_timeline=PASS")
print("hosted_execution_graph_r1=PASS")
PY

echo
echo "=== SERVER SHUTDOWN / RESTART ==="
stop_server
sleep 0.2
start_server
fetch_json "/health" "health-restart"
fetch_json "/semantics" "semantics-restart"

python3 - "$OUT/health-restart.json" "$OUT/semantics-restart.json" <<'PY'
import json
import sys
health = json.load(open(sys.argv[1], encoding="utf-8"))
semantics = json.load(open(sys.argv[2], encoding="utf-8"))
assert health["status"] == "ok"
assert semantics["packages"][0]["resolved_count"] == 2
assert semantics["packages"][0]["missing_count"] == 9
print("hosted_restart=PASS")
print("hosted_semantic_registry_restart=PASS")
PY

stop_server

if pgrep -f "$PREFIX/bin/air-server" >/dev/null 2>&1; then
    echo "ERROR: installed air-server process remained after hosted hardening" >&2
    pgrep -af "$PREFIX/bin/air-server" >&2 || true
    exit 6
fi

echo
echo "=== HOSTED HARDENING SUMMARY ==="
{
    echo "source=$(git -C "$ROOT" rev-parse HEAD)"
    echo "version=$("$PREFIX/bin/air-cli" --version 2>&1 | head -n1)"
    echo "web_version=$(sed -n 's/.*air-web-version\" content=\"\([^\"]*\)\".*/\1/p' "$PREFIX/share/air/web/index.html" | head -n1)"
    echo "model_sha256=$(sha256sum "$MODEL" | awk '{print $1}')"
    echo "backend=reference"
    echo "gpu_qualified=false"
    echo "human_usability_qualified=false"
} | tee "$OUT/summary.txt"

(
    cd "$OUT"
    find . -maxdepth 1 -type f -print0 |
        sort -z |
        xargs -0 sha256sum
) > "$OUT/SHA256SUMS.txt"

echo
echo "PROMPT11_HOSTED_RC_HARDENING=PASS"
echo "Evidence directory: $OUT"
