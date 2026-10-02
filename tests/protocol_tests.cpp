#include "../src/server/protocol.hpp"
#include <memory>

#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const std::string& message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
}

int main() {
    auto native = air::server::parse_generation_request(
        "/generate", R"({"prompt":"hello","max_tokens":7,"temperature":0.2,"top_p":0.9,"stream":true})");
    check(native && native.value().stream && native.value().inference.prompt == "hello" &&
          native.value().inference.generation.max_new_tokens == 7,
          "native generation request is parsed once into canonical inference request");

    auto chat = air::server::parse_generation_request(
        "/v1/chat/completions",
        R"({"model":"x","messages":[{"role":"system","content":"s"},{"role":"user","content":"u"}],"max_completion_tokens":9,"stream":false})");
    check(chat && chat.value().inference.messages.size() == 2 &&
          chat.value().inference.generation.max_new_tokens == 9,
          "chat-completions request maps messages and max_completion_tokens");

    auto unsupported = air::server::parse_generation_request(
        "/v1/chat/completions",
        R"({"messages":[{"role":"user","content":"u"}],"tools":[]})");
    check(!unsupported && unsupported.status().code() == air::ErrorCode::unsupported,
          "unimplemented OpenAI fields fail explicitly rather than being ignored");

    auto seeded = air::server::parse_generation_request(
        "/v1/completions",
        R"({"model":"x","prompt":"hello","max_tokens":4,"temperature":0.7,"seed":18446744073709551615,"n":1})");
    check(seeded && seeded.value().inference.generation.sampling.seed == 18446744073709551615ULL,
          "OpenAI-shaped requests preserve the full uint64 sampling seed");

    auto unknown_field = air::server::parse_generation_request(
        "/v1/completions", R"({"prompt":"hello","frequency_penalty":0.2})");
    check(!unknown_field && unknown_field.status().code() == air::ErrorCode::unsupported,
          "unknown compatibility fields are rejected instead of silently ignored");

    auto unknown_message_field = air::server::parse_generation_request(
        "/v1/chat/completions",
        R"({"messages":[{"role":"user","content":"u","name":"ignored-before-lock6"}]})");
    check(!unknown_message_field && unknown_message_field.status().code() == air::ErrorCode::unsupported,
          "unknown chat-message fields are rejected instead of silently ignored");

    auto multiple = air::server::parse_generation_request(
        "/v1/completions", R"({"prompt":"hello","n":2})");
    check(!multiple && multiple.status().code() == air::ErrorCode::unsupported,
          "completion compatibility contract explicitly limits n to one");

    auto conflicting_max = air::server::parse_generation_request(
        "/v1/chat/completions",
        R"({"messages":[{"role":"user","content":"u"}],"max_tokens":4,"max_completion_tokens":5})");
    check(!conflicting_max && conflicting_max.status().code() == air::ErrorCode::invalid_argument,
          "conflicting generation limits fail explicitly");

    air::InferenceResponse response;
    response.text = "ok";
    response.tokens = {1, 2};
    response.metrics.prompt_tokens = 3;
    response.metrics.generated_tokens = 2;
    const auto json = air::server::completion_json(chat.value(), response);
    check(json.find("chat.completion") != std::string::npos && json.find("\"usage\"") != std::string::npos,
          "chat completion response includes OpenAI-shaped object and usage");

    auto model = [] {
        air::ModelConfig config;
        config.architecture = "qwen2";
        config.layer_count = 1;
        config.embedding_size = 4;
        config.feed_forward_size = 8;
        config.attention_head_count = 2;
        config.kv_head_count = 1;
        config.rope_dimension_count = 2;
        config.context_length = 32;
        config.vocabulary_size = 1;
        air::TokenizerDefinition tokenizer;
        tokenizer.model = "gpt2";
        tokenizer.pre_tokenizer = "gpt2";
        tokenizer.vocabulary = {"x"};
        tokenizer.token_types = {1};
        return air::ModelDefinition(
            air::ModelFingerprint{"test", "qwen2", "model\"id"},
            std::move(config), std::move(tokenizer), {}, {});
    }();
    const auto models = air::server::models_json(model);
    check(models.find("model\\\"id") != std::string::npos,
          "models endpoint JSON-escapes arbitrary model identifiers");

    air::ServiceSnapshot snapshot;
    snapshot.max_prefill_batch_width = 128;
    snapshot.max_decode_batch_width = 1;
    snapshot.device_greedy_selection = true;
    snapshot.cancelled_requests = 3;
    snapshot.stream_delivery_failures = 2;
    snapshot.current_kv_bytes = 4096;
    snapshot.stream_queue_capacity = 64;
    const auto runtime = air::server::service_snapshot_json(snapshot);
    check(runtime.find("\"max_prefill_batch_width\":128") != std::string::npos &&
          runtime.find("\"max_decode_batch_width\":1") != std::string::npos,
          "runtime snapshot exposes prefill and decode execution widths separately");
    check(runtime.find("\"device_greedy_selection\":true") != std::string::npos,
          "runtime snapshot exposes exact device greedy-selection capability");
    check(runtime.find("\"cancelled_requests\":3") != std::string::npos &&
          runtime.find("\"stream_delivery_failures\":2") != std::string::npos &&
          runtime.find("\"current_kv_bytes\":4096") != std::string::npos &&
          runtime.find("\"stream_queue_capacity\":64") != std::string::npos,
          "runtime snapshot exposes cancellation, delivery, live-KV, and stream-backpressure telemetry");


    auto decision = air::server::parse_decision_request(
        R"({"input":"route:","candidates":[{"id":"billing","text":"Billing","model_text":" billing"},{"id":"tech","text":"Technical","model_text":" technical"}],"scoring_policy":"sequence-logprob-mean","output_cardinality":"exactly-one","determinism":"required"})");
    check(decision &&
          decision.value().decision.candidates.size() == 2 &&
          decision.value().decision.candidates[0].id == "billing" &&
          decision.value().decision.scoring_policy ==
              air::DecisionScoringPolicy::sequence_logprob_mean,
          "native decision endpoint preserves semantic candidate IDs and explicit scoring policy");

    auto decision_auto = air::server::parse_decision_request(
        R"({"input":"x","candidates":[{"id":"a","text":"A"}]})");
    check(decision_auto &&
          decision_auto.value().decision.scoring_policy ==
              air::DecisionScoringPolicy::qualified_auto,
          "decision parser preserves contract default qualified-auto for service-level qualification");

    auto decision_unknown = air::server::parse_decision_request(
        R"({"input":"x","candidates":[{"id":"a","text":"A"}],"confidence":true})");
    check(!decision_unknown &&
          decision_unknown.status().code() == air::ErrorCode::unsupported,
          "decision endpoint rejects invented confidence/probability controls");

    air::DecisionResponse decision_response;
    decision_response.decision.selected_candidate_ids = {"billing"};
    decision_response.decision.scores = {
        {"billing", 0.75}, {"tech", 0.25}
    };
    decision_response.decision.applied_scoring_policy =
        air::DecisionScoringPolicy::sequence_logprob_mean;
    decision_response.metrics.workload = "decision";
    decision_response.metrics.prompt_tokens = 3;
    decision_response.candidate_tokens_scored = 4;
    decision_response.branch_count = 2;
    const auto decision_body =
        air::server::decision_json(decision_response);
    check(decision_body.find("\"calibrated\":false") != std::string::npos &&
          decision_body.find("\"score_semantics\":\"candidate-set-normalized\"") !=
              std::string::npos &&
          decision_body.find("\"normalized_score\":") != std::string::npos &&
          decision_body.find("\"confidence\"") == std::string::npos &&
          decision_body.find("\"probability\"") == std::string::npos,
          "decision response exposes normalized candidate-set scores without calling them confidence");

    snapshot.rejected_overload_requests = 5;
    snapshot.completed_decisions = 7;
    snapshot.max_queued_requests = 64;
    const auto runtime_with_decisions =
        air::server::service_snapshot_json(snapshot);
    check(runtime_with_decisions.find("\"rejected_overload_requests\":5") !=
              std::string::npos &&
          runtime_with_decisions.find("\"completed_decisions\":7") !=
              std::string::npos &&
          runtime_with_decisions.find("\"max_queued_requests\":64") !=
              std::string::npos,
          "runtime snapshot exposes mixed-workload backpressure and decision completion telemetry");

    const auto metrics_with_decisions =
        air::server::metrics_text(snapshot);
    check(metrics_with_decisions.find("air_completed_decisions_total 7") !=
              std::string::npos &&
          metrics_with_decisions.find("air_rejected_overload_requests_total 5") !=
              std::string::npos,
          "Prometheus metrics expose decision and overload counters");

    air::ExecutionTimelineSnapshot timeline;
    timeline.level = air::ExecutionObservationLevel::normal;
    timeline.origin_unix_ms = 1234U;
    timeline.capacity = 16U;
    timeline.evicted_spans = 2U;
    air::ExecutionSpan span;
    span.observation_sequence = 7U;
    span.request_id = 3U;
    span.sequence_id = 4U;
    span.scope = air::ExecutionSpanScope::service;
    span.category = air::ExecutionSpanCategory::backend_call;
    span.phase = "prefill";
    span.backend = "reference";
    span.start_ns = 10U;
    span.end_ns = 20U;
    span.work_unit_kind = air::WorkUnitKind::tokens;
    span.work_units = 2U;
    timeline.spans.push_back(span);
    const auto timeline_body = air::server::execution_timeline_json(timeline);
    check(timeline_body.find("\"level\":\"normal\"") != std::string::npos &&
          timeline_body.find("\"clock\":\"steady_clock\"") != std::string::npos &&
          timeline_body.find("\"category\":\"backend-call\"") != std::string::npos &&
          timeline_body.find("\"phase\":\"prefill\"") != std::string::npos &&
          timeline_body.find("\"start_ns\":10") != std::string::npos &&
          timeline_body.find("\"end_ns\":20") != std::string::npos &&
          timeline_body.find("\"work_unit_kind\":\"tokens\"") != std::string::npos,
          "execution timeline serialization preserves typed clock and span semantics");

    air::BackendCapabilities graph_capabilities;
    graph_capabilities.backend = air::BackendKind::reference;
    graph_capabilities.kv_storage = air::KvStorageKind::paged;

    air::ExecutionPlan graph_plan;
    graph_plan.backend = air::BackendKind::reference;
    graph_plan.strategy_id = "protocol-graph";
    graph_plan.scheduling.prefill_quantum_tokens = 2U;
    graph_plan.kv.page_tokens = 2U;

    air::PhysicalInvocation invocation;
    invocation.kind = air::PhysicalInvocationKind::prefill_single;
    invocation.work_unit_kind = air::WorkUnitKind::tokens;
    invocation.participants.push_back(
        air::PhysicalInvocationParticipant{
            2U, air::PhysicalOutputMode::logits, 0U});
    invocation.topology_fingerprint = "hardware-topology:protocol";
    invocation.hardware_resource_id = "cpu0";
    auto graph = air::derive_execution_graph(
        graph_plan, graph_capabilities, invocation);
    check(graph.is_ok(), "protocol fixture derives an ExecutionGraph");
    if (graph) {
        air::ExecutionGraphTimelineSnapshot graph_timeline;
        graph_timeline.level = air::ExecutionObservationLevel::detailed;
        graph_timeline.capacity = 8U;
        graph_timeline.topology_status = "ready";
        graph_timeline.topology_fingerprint =
            invocation.topology_fingerprint;

        air::ExecutionGraphObservation graph_observation;
        graph_observation.observation_sequence = 3U;
        graph_observation.graph =
            std::make_shared<const air::ExecutionGraph>(
                std::move(graph).value());
        graph_observation.participants.push_back({11U, 12U});
        graph_observation.start_ns = 30U;
        graph_observation.end_ns = 40U;
        graph_observation.backend_success = true;
        graph_observation.evidence_status =
            air::ExecutionGraphEvidenceStatus::concordant;
        graph_timeline.observations.push_back(
            std::move(graph_observation));

        const auto graph_body =
            air::server::execution_graph_timeline_json(
                graph_timeline);
        check(graph_body.find("\"topology_status\":\"ready\"") !=
                  std::string::npos &&
              graph_body.find("\"evidence_status\":\"concordant\"") !=
                  std::string::npos &&
              graph_body.find("\"schema_version\":3") !=
                  std::string::npos &&
              graph_body.find("\"identity\":\"execution-graph:r1:") !=
                  std::string::npos &&
              graph_body.find("\"workload_kind\":\"autoregressive-tokens\"") !=
                  std::string::npos &&
              graph_body.find("\"invocation\":\"prefill-single\"") !=
                  std::string::npos &&
              graph_body.find("\"work_unit_kind\":\"tokens\"") !=
                  std::string::npos &&
              graph_body.find("\"hardware_resource_id\":\"cpu0\"") !=
                  std::string::npos &&
              graph_body.find("\"item_count\":0") !=
                  std::string::npos &&
              graph_body.find("\"prepared_resource_ids\":[]") !=
                  std::string::npos &&
              graph_body.find("\"input_value_ids\":[]") !=
                  std::string::npos &&
              graph_body.find("\"output_value_ids\":[]") !=
                  std::string::npos &&
              graph_body.find("\"request_id\":11") !=
                  std::string::npos,
              "execution graph endpoint serializes planned graph and evidence correlation as separate structured data");
    }

    if (failures != 0) {
        std::cerr << failures << " protocol test(s) failed\n";
        return 1;
    }
    std::cout << "protocol tests passed\n";
    return 0;
}
