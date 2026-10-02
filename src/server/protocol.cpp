#include "protocol.hpp"

#include <boost/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>

namespace air::server {
namespace json = boost::json;
namespace {

std::atomic<std::uint64_t> next_response_id{1};

[[nodiscard]] std::uint64_t epoch_seconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

[[nodiscard]] Result<json::object> parse_object(std::string_view body) {
    boost::system::error_code error;
    auto value = json::parse(body, error);
    if (error) return Status::invalid_argument("request body is not valid JSON: " + error.message());
    if (!value.is_object()) return Status::invalid_argument("request body must be a JSON object");
    return value.as_object();
}

[[nodiscard]] Result<double> number(const json::object& object, std::string_view key, double fallback) {
    const auto* value = object.if_contains(key);
    if (!value) return fallback;
    if (value->is_double()) return value->as_double();
    if (value->is_int64()) return static_cast<double>(value->as_int64());
    if (value->is_uint64()) return static_cast<double>(value->as_uint64());
    return Status::invalid_argument(std::string(key) + " must be numeric");
}

[[nodiscard]] Result<std::uint32_t> u32(const json::object& object,
                                        std::string_view key,
                                        std::uint32_t fallback) {
    const auto* value = object.if_contains(key);
    if (!value) return fallback;
    std::uint64_t raw = 0;
    if (value->is_uint64()) raw = value->as_uint64();
    else if (value->is_int64() && value->as_int64() >= 0) raw = static_cast<std::uint64_t>(value->as_int64());
    else return Status::invalid_argument(std::string(key) + " must be a non-negative integer");
    if (raw > std::numeric_limits<std::uint32_t>::max()) {
        return Status::invalid_argument(std::string(key) + " exceeds uint32 range");
    }
    return static_cast<std::uint32_t>(raw);
}

[[nodiscard]] Result<std::uint64_t> u64(const json::object& object,
                                        std::string_view key,
                                        std::uint64_t fallback) {
    const auto* value = object.if_contains(key);
    if (!value) return fallback;
    if (value->is_uint64()) return value->as_uint64();
    if (value->is_int64() && value->as_int64() >= 0) {
        return static_cast<std::uint64_t>(value->as_int64());
    }
    return Status::invalid_argument(std::string(key) + " must be a non-negative integer");
}

[[nodiscard]] bool allowed_key(std::string_view key,
                               std::initializer_list<std::string_view> allowed) {
    for (const auto candidate : allowed) {
        if (candidate == key) return true;
    }
    return false;
}

[[nodiscard]] Status reject_unknown_fields(
    const json::object& object, std::initializer_list<std::string_view> allowed) {
    for (const auto& item : object) {
        if (!allowed_key(item.key(), allowed)) {
            return Status::unsupported("AIR does not implement request field: " +
                                       std::string(item.key()));
        }
    }
    return Status::ok();
}

[[nodiscard]] Status validate_optional_model(const json::object& object) {
    const auto* value = object.if_contains("model");
    if (!value) return Status::ok();
    if (!value->is_string()) return Status::invalid_argument("model must be a string");
    return Status::ok();
}

[[nodiscard]] Status validate_optional_n_one(const json::object& object) {
    const auto* value = object.if_contains("n");
    if (!value) return Status::ok();
    if (value->is_int64() && value->as_int64() == 1) return Status::ok();
    if (value->is_uint64() && value->as_uint64() == 1U) return Status::ok();
    return Status::unsupported("AIR currently supports n=1 only");
}

[[nodiscard]] Result<bool> boolean(const json::object& object, std::string_view key, bool fallback) {
    const auto* value = object.if_contains(key);
    if (!value) return fallback;
    if (!value->is_bool()) return Status::invalid_argument(std::string(key) + " must be boolean");
    return value->as_bool();
}

[[nodiscard]] Result<std::string> required_string(const json::object& object, std::string_view key) {
    const auto* value = object.if_contains(key);
    if (!value || !value->is_string()) return Status::invalid_argument(std::string(key) + " must be a string");
    return std::string(value->as_string());
}


[[nodiscard]] Result<GenerationConfig> parse_generation(const json::object& object, ApiFlavor flavor) {
    GenerationConfig config;
    if (object.if_contains("max_tokens") && object.if_contains("max_completion_tokens")) {
        return Status::invalid_argument("specify only one of max_tokens or max_completion_tokens");
    }
    const auto max_key = flavor == ApiFlavor::native ? "max_tokens" :
                         (object.if_contains("max_completion_tokens") ? "max_completion_tokens" : "max_tokens");
    auto max_tokens = u32(object, max_key, 32U);
    auto temperature = number(object, "temperature", 0.0);
    auto top_p = number(object, "top_p", 1.0);
    auto top_k = u32(object, "top_k", 0U);
    auto seed = u64(object, "seed", 0U);
    if (!max_tokens) return max_tokens.status();
    if (!temperature) return temperature.status();
    if (!top_p) return top_p.status();
    if (!top_k) return top_k.status();
    if (!seed) return seed.status();
    if (!std::isfinite(temperature.value()) || temperature.value() < 0.0) {
        return Status::invalid_argument("temperature must be finite and non-negative");
    }
    if (!std::isfinite(top_p.value()) || top_p.value() <= 0.0 || top_p.value() > 1.0) {
        return Status::invalid_argument("top_p must be in (0, 1]");
    }
    config.max_new_tokens = max_tokens.value();
    config.sampling.temperature = temperature.value();
    config.sampling.top_p = top_p.value();
    config.sampling.top_k = top_k.value();
    config.sampling.seed = seed.value();
    return config;
}

[[nodiscard]] json::object usage_json(const InferenceResponse& response) {
    json::object usage;
    usage["prompt_tokens"] = response.metrics.prompt_tokens;
    usage["completion_tokens"] = response.metrics.generated_tokens;
    usage["total_tokens"] = response.metrics.prompt_tokens + response.metrics.generated_tokens;
    return usage;
}

[[nodiscard]] json::object metrics_json(const RequestMetrics& metrics) {
    json::object out;
    out["request_id"] = metrics.request_id;
    out["sequence_id"] = metrics.sequence_id;
    out["backend"] = metrics.backend;
    out["workload"] = metrics.workload;
    out["planner_mode"] = metrics.planner_mode;
    out["strategy_id"] = metrics.strategy_id;
    out["strategy_objective"] = metrics.strategy_objective;
    out["strategy_decision_reason"] = metrics.strategy_decision_reason;
    out["strategy_eligible_candidates"] = metrics.strategy_eligible_candidates;
    out["strategy_prepared_state_hot"] = metrics.strategy_prepared_state_hot;
    out["strategy_estimated_transition_ms"] = metrics.strategy_estimated_transition_ms;
    out["strategy_estimated_break_even_tokens"] = metrics.strategy_estimated_break_even_tokens;
    json::array strategy_candidates;
    for (const auto& c : metrics.strategy_candidates) {
        json::object entry;
        entry["strategy_id"] = c.strategy_id;
        entry["disposition"] = c.disposition;
        entry["eligible"] = c.eligible;
        entry["memory_feasible"] = c.memory_feasible;
        entry["prepared_state_hot"] = c.prepared_state_hot;
        entry["estimated_transition_ms"] = c.estimated_transition_ms;
        entry["estimated_horizon_ms"] = c.estimated_horizon_ms;
        entry["estimated_break_even_tokens"] = c.estimated_break_even_tokens;
        entry["prepared_artifact_bytes"] = c.prepared_artifact_bytes;
        strategy_candidates.push_back(std::move(entry));
    }
    out["strategy_candidates"] = std::move(strategy_candidates);
    out["prompt_tokens"] = metrics.prompt_tokens;
    out["generated_tokens"] = metrics.generated_tokens;
    out["prefix_reused_tokens"] = metrics.prefix_reused_tokens;
    out["kv_bytes"] = metrics.kv_bytes;
    out["plan_preparation_bytes"] = metrics.plan_preparation_bytes;
    out["plan_preparation_ms"] = metrics.plan_preparation_ms;
    out["plan_eviction_ms"] = metrics.plan_eviction_ms;
    out["queue_ms"] = metrics.queue_ms;
    out["prefill_ms"] = metrics.prefill_ms;
    out["ttft_ms"] = metrics.ttft_ms;
    out["decode_ms"] = metrics.decode_ms;
    out["total_ms"] = metrics.total_ms;
    out["prefill_tokens_per_second"] = metrics.prefill_tokens_per_second();
    out["decode_tokens_per_second"] = metrics.decode_tokens_per_second();
    return out;
}

} // namespace

Result<ParsedRequest> parse_generation_request(std::string_view target, std::string_view body) {
    auto parsed = parse_object(body);
    if (!parsed) return parsed.status();
    const auto& object = parsed.value();

    ParsedRequest request;
    request.created = epoch_seconds();
    request.response_id = "air-" + std::to_string(next_response_id.fetch_add(1));
    if (target == "/generate") request.flavor = ApiFlavor::native;
    else if (target == "/v1/completions") request.flavor = ApiFlavor::completions;
    else if (target == "/v1/chat/completions") request.flavor = ApiFlavor::chat_completions;
    else return Status::invalid_argument("unknown generation endpoint");

    auto stream = boolean(object, "stream", false);
    if (!stream) return stream.status();
    request.stream = stream.value();
    auto generation = parse_generation(object, request.flavor);
    if (!generation) return generation.status();
    request.inference.generation = generation.value();

    if (request.flavor == ApiFlavor::native) {
        const auto fields = reject_unknown_fields(
            object, {"prompt", "max_tokens", "temperature", "top_p", "top_k", "seed", "stream"});
        if (!fields) return fields;
        auto prompt = required_string(object, "prompt");
        if (!prompt) return prompt.status();
        request.inference.prompt = std::move(prompt).value();
    } else if (request.flavor == ApiFlavor::chat_completions) {
        const auto fields = reject_unknown_fields(
            object, {"model", "messages", "max_tokens", "max_completion_tokens", "temperature",
                     "top_p", "top_k", "seed", "stream", "n"});
        if (!fields) return fields;
        const auto model_status = validate_optional_model(object);
        if (!model_status) return model_status;
        const auto n_status = validate_optional_n_one(object);
        if (!n_status) return n_status;
        const auto* messages = object.if_contains("messages");
        if (!messages || !messages->is_array()) return Status::invalid_argument("messages must be an array");
        for (const auto& value : messages->as_array()) {
            if (!value.is_object()) return Status::invalid_argument("each chat message must be an object");
            const auto message_fields = reject_unknown_fields(value.as_object(), {"role", "content"});
            if (!message_fields) return message_fields;
            auto role = required_string(value.as_object(), "role");
            auto content = required_string(value.as_object(), "content");
            if (!role) return role.status();
            if (!content) return content.status();
            request.inference.messages.push_back({std::move(role).value(), std::move(content).value()});
        }
    } else {
        const auto fields = reject_unknown_fields(
            object, {"model", "prompt", "max_tokens", "max_completion_tokens", "temperature",
                     "top_p", "top_k", "seed", "stream", "n"});
        if (!fields) return fields;
        const auto model_status = validate_optional_model(object);
        if (!model_status) return model_status;
        const auto n_status = validate_optional_n_one(object);
        if (!n_status) return n_status;
        auto prompt = required_string(object, "prompt");
        if (!prompt) return prompt.status();
        request.inference.prompt = std::move(prompt).value();
    }
    return request;
}

Result<ParsedDecisionRequest> parse_decision_request(std::string_view body) {
    auto parsed = parse_object(body);
    if (!parsed) return parsed.status();
    const auto& object = parsed.value();

    const auto fields = reject_unknown_fields(
        object, {"input", "candidates", "scoring_policy",
                 "output_cardinality", "determinism"});
    if (!fields) return fields;

    ParsedDecisionRequest out;
    auto input = required_string(object, "input");
    if (!input) return input.status();
    out.decision.input_text = std::move(input).value();

    const auto* candidates = object.if_contains("candidates");
    if (!candidates || !candidates->is_array()) {
        return Status::invalid_argument("candidates must be an array");
    }
    for (const auto& value : candidates->as_array()) {
        if (!value.is_object()) {
            return Status::invalid_argument("each decision candidate must be an object");
        }
        const auto& candidate = value.as_object();
        const auto candidate_fields =
            reject_unknown_fields(candidate, {"id", "text", "model_text"});
        if (!candidate_fields) return candidate_fields;

        auto id = required_string(candidate, "id");
        auto text = required_string(candidate, "text");
        if (!id) return id.status();
        if (!text) return text.status();

        DecisionCandidate parsed_candidate;
        parsed_candidate.id = std::move(id).value();
        parsed_candidate.display_text = std::move(text).value();
        if (const auto* model_text = candidate.if_contains("model_text")) {
            if (!model_text->is_string()) {
                return Status::invalid_argument("candidate model_text must be a string");
            }
            parsed_candidate.model_text = std::string(model_text->as_string());
        }
        out.decision.candidates.push_back(std::move(parsed_candidate));
    }

    if (const auto* scoring = object.if_contains("scoring_policy")) {
        if (!scoring->is_string()) {
            return Status::invalid_argument("scoring_policy must be a string");
        }
        auto policy = decision_scoring_policy_from_string(
            std::string_view(scoring->as_string().data(), scoring->as_string().size()));
        if (!policy) return policy.status();
        out.decision.scoring_policy = policy.value();
    }

    if (const auto* cardinality = object.if_contains("output_cardinality")) {
        if (!cardinality->is_string()) {
            return Status::invalid_argument("output_cardinality must be a string");
        }
        auto parsed_cardinality = decision_output_cardinality_from_string(
            std::string_view(cardinality->as_string().data(),
                             cardinality->as_string().size()));
        if (!parsed_cardinality) return parsed_cardinality.status();
        out.decision.output_cardinality = parsed_cardinality.value();
    }

    if (const auto* determinism = object.if_contains("determinism")) {
        if (!determinism->is_string()) {
            return Status::invalid_argument("determinism must be a string");
        }
        auto parsed_determinism = determinism_requirement_from_string(
            std::string_view(determinism->as_string().data(),
                             determinism->as_string().size()));
        if (!parsed_determinism) return parsed_determinism.status();
        out.decision.determinism = parsed_determinism.value();
    }

    const auto valid = validate_decision_request(out.decision);
    if (!valid) return valid;
    return out;
}

std::string decision_json(const DecisionResponse& response) {
    json::object root;
    json::array selected;
    for (const auto& id : response.decision.selected_candidate_ids) {
        selected.push_back(json::value(id));
    }
    root["selected_candidate_ids"] = std::move(selected);

    json::array scores;
    for (const auto& score : response.decision.scores) {
        json::object entry;
        entry["candidate_id"] = score.candidate_id;
        entry["normalized_score"] = score.normalized_score;
        scores.push_back(std::move(entry));
    }
    root["scores"] = std::move(scores);
    root["applied_scoring_policy"] =
        to_string(response.decision.applied_scoring_policy);
    root["score_semantics"] =
        to_string(response.decision.score_semantics);
    root["calibrated"] = response.calibrated;
    root["abstention_qualified"] = response.abstention_qualified;
    root["candidate_tokens_scored"] = response.candidate_tokens_scored;
    root["branch_count"] = response.branch_count;
    root["metrics"] = metrics_json(response.metrics);
    return json::serialize(root);
}

std::string completion_json(const ParsedRequest& request, const InferenceResponse& response) {
    json::object root;
    root["id"] = request.response_id;
    root["created"] = request.created;
    if (request.flavor == ApiFlavor::native) {
        root["text"] = response.text;
        root["finish_reason"] = response.hit_eos ? "stop" : "length";
        root["usage"] = usage_json(response);
        root["metrics"] = metrics_json(response.metrics);
    } else if (request.flavor == ApiFlavor::completions) {
        root["object"] = "text_completion";
        json::object choice;
        choice["index"] = 0;
        choice["text"] = response.text;
        choice["finish_reason"] = response.hit_eos ? "stop" : "length";
        root["choices"] = json::array{std::move(choice)};
        root["usage"] = usage_json(response);
    } else {
        root["object"] = "chat.completion";
        json::object message;
        message["role"] = "assistant";
        message["content"] = response.text;
        json::object choice;
        choice["index"] = 0;
        choice["message"] = std::move(message);
        choice["finish_reason"] = response.hit_eos ? "stop" : "length";
        root["choices"] = json::array{std::move(choice)};
        root["usage"] = usage_json(response);
    }
    return json::serialize(root);
}

std::string stream_chunk_json(const ParsedRequest& request,
                              std::string_view delta,
                              TokenId,
                              bool first_chunk) {
    json::object root;
    root["id"] = request.response_id;
    root["created"] = request.created;
    if (request.flavor == ApiFlavor::native) {
        root["text"] = delta;
    } else if (request.flavor == ApiFlavor::completions) {
        root["object"] = "text_completion";
        json::object choice;
        choice["index"] = 0;
        choice["text"] = delta;
        choice["finish_reason"] = nullptr;
        root["choices"] = json::array{std::move(choice)};
    } else {
        root["object"] = "chat.completion.chunk";
        json::object delta_object;
        if (first_chunk) delta_object["role"] = "assistant";
        if (!delta.empty()) delta_object["content"] = delta;
        json::object choice;
        choice["index"] = 0;
        choice["delta"] = std::move(delta_object);
        choice["finish_reason"] = nullptr;
        root["choices"] = json::array{std::move(choice)};
    }
    return json::serialize(root);
}

std::string stream_finish_json(const ParsedRequest& request, const InferenceResponse& response) {
    json::object root;
    root["id"] = request.response_id;
    root["created"] = request.created;
    if (request.flavor == ApiFlavor::native) {
        root["done"] = true;
        root["finish_reason"] = response.hit_eos ? "stop" : "length";
        root["usage"] = usage_json(response);
        root["metrics"] = metrics_json(response.metrics);
    } else if (request.flavor == ApiFlavor::completions) {
        root["object"] = "text_completion";
        json::object choice;
        choice["index"] = 0;
        choice["text"] = "";
        choice["finish_reason"] = response.hit_eos ? "stop" : "length";
        root["choices"] = json::array{std::move(choice)};
        root["usage"] = usage_json(response);
    } else {
        root["object"] = "chat.completion.chunk";
        json::object choice;
        choice["index"] = 0;
        choice["delta"] = json::object{};
        choice["finish_reason"] = response.hit_eos ? "stop" : "length";
        root["choices"] = json::array{std::move(choice)};
        root["usage"] = usage_json(response);
    }
    return json::serialize(root);
}

std::string error_json(std::string_view message, std::string_view type) {
    json::object error;
    error["message"] = message;
    error["type"] = type;
    error["code"] = nullptr;
    json::object root;
    root["error"] = std::move(error);
    return json::serialize(root);
}

std::string service_snapshot_json(const ServiceSnapshot& snapshot) {
    json::object out;
    out["backend"] = snapshot.backend;
    json::object planner;
    planner["mode"] = snapshot.planner_mode;
    planner["manifest_status"] = snapshot.manifest_status;
    planner["manifest_id"] = snapshot.manifest_id;
    planner["strategy_id"] = snapshot.strategy_id;
    planner["objective"] = snapshot.strategy_objective;
    planner["decision_reason"] = snapshot.strategy_decision_reason;
    planner["eligible_candidates"] = snapshot.strategy_eligible_candidates;
    planner["prepared_state_hot"] = snapshot.strategy_prepared_state_hot;
    planner["estimated_transition_ms"] = snapshot.strategy_estimated_transition_ms;
    planner["estimated_break_even_tokens"] = snapshot.strategy_estimated_break_even_tokens;
    json::array candidate_trace;
    for (const auto& c : snapshot.strategy_candidates) {
        json::object entry;
        entry["strategy_id"] = c.strategy_id;
        entry["disposition"] = c.disposition;
        entry["eligible"] = c.eligible;
        entry["memory_feasible"] = c.memory_feasible;
        entry["prepared_state_hot"] = c.prepared_state_hot;
        entry["estimated_transition_ms"] = c.estimated_transition_ms;
        entry["estimated_horizon_ms"] = c.estimated_horizon_ms;
        entry["estimated_break_even_tokens"] = c.estimated_break_even_tokens;
        entry["prepared_artifact_bytes"] = c.prepared_artifact_bytes;
        candidate_trace.push_back(std::move(entry));
    }
    planner["candidates"] = std::move(candidate_trace);
    planner["selected_backend"] = snapshot.selected_backend;
    planner["prefill_quantum_tokens"] = snapshot.planned_prefill_quantum_tokens;
    planner["kv_page_tokens"] = snapshot.planned_kv_page_tokens;
    planner["prefill_block_quantized_linear"] = snapshot.planned_prefill_block_linear_tactic;
    planner["decode_block_quantized_linear"] = snapshot.planned_decode_block_linear_tactic;
    planner["decode_output_quantized_linear"] = snapshot.planned_decode_output_linear_tactic;
    planner["prefill_attention"] = snapshot.planned_prefill_attention_tactic;
    planner["decode_attention"] = snapshot.planned_decode_attention_tactic;
    out["planner"] = std::move(planner);
    out["queued_requests"] = snapshot.queued_requests;
    out["active_requests"] = snapshot.active_requests;
    out["rejected_overload_requests"] = snapshot.rejected_overload_requests;
    out["completed_decisions"] = snapshot.completed_decisions;
    out["max_queued_requests"] = snapshot.max_queued_requests;
    out["completed_requests"] = snapshot.completed_requests;
    out["failed_requests"] = snapshot.failed_requests;
    out["cancelled_requests"] = snapshot.cancelled_requests;
    out["stream_delivery_failures"] = snapshot.stream_delivery_failures;
    out["total_prompt_tokens"] = snapshot.total_prompt_tokens;
    out["total_generated_tokens"] = snapshot.total_generated_tokens;
    out["total_prefix_reused_tokens"] = snapshot.total_prefix_reused_tokens;
    out["peak_kv_bytes"] = snapshot.peak_kv_bytes;
    out["peak_device_bytes"] = snapshot.peak_device_bytes;
    out["current_kv_bytes"] = snapshot.current_kv_bytes;
    out["current_device_bytes"] = snapshot.current_device_bytes;
    out["current_prepared_artifact_bytes"] = snapshot.current_prepared_artifact_bytes;
    json::array prepared_resources;
    for (const auto& resource : snapshot.current_prepared_resources) {
        json::object item;
        item["resource_id"] = resource.resource_id;
        item["state"] = std::string(to_string(resource.state));
        item["device_bytes"] = resource.device_bytes;
        prepared_resources.push_back(std::move(item));
    }
    out["current_prepared_resources"] = std::move(prepared_resources);
    out["aggregate_generated_tokens_per_second"] = snapshot.aggregate_generated_tokens_per_second;
    out["p50_total_ms"] = snapshot.p50_total_ms;
    out["p95_total_ms"] = snapshot.p95_total_ms;
    json::object scheduler;
    scheduler["max_active_requests"] = snapshot.max_active_requests;
    scheduler["token_budget_per_cycle"] = snapshot.token_budget_per_cycle;
    scheduler["prefill_quantum_tokens"] = snapshot.prefill_quantum_tokens;
    scheduler["reference_kv_page_tokens"] = snapshot.reference_kv_page_tokens;
    scheduler["cuda_kv_page_tokens"] = snapshot.cuda_kv_page_tokens;
    scheduler["max_prefill_batch_width"] = snapshot.max_prefill_batch_width;
    scheduler["max_decode_batch_width"] = snapshot.max_decode_batch_width;
    scheduler["stream_queue_capacity"] = snapshot.stream_queue_capacity;
    out["scheduler"] = std::move(scheduler);
    json::object capabilities;
    capabilities["physical_paged_kv"] = snapshot.physical_paged_kv;
    capabilities["prefill_execution"] = snapshot.prefill_execution;
    capabilities["kv_storage"] = snapshot.kv_storage;
    capabilities["admission_reserved_bytes"] = snapshot.admission_reserved_bytes;
    capabilities["admission_capacity_bytes"] = snapshot.admission_capacity_bytes;
    capabilities["kv_pool_allocated_bytes"] = snapshot.kv_pool_allocated_bytes;
    capabilities["kv_pool_free_bytes"] = snapshot.kv_pool_free_bytes;
    capabilities["sequence_checkpointing"] = snapshot.sequence_checkpointing;
    capabilities["prefix_cache_enabled"] = snapshot.prefix_cache_enabled;
    capabilities["device_greedy_selection"] = snapshot.device_greedy_selection;
    out["capabilities"] = std::move(capabilities);
    json::object state_store;
    state_store["capacity_entries"] = snapshot.sequence_state_store_capacity_entries;
    state_store["entries"] = snapshot.sequence_state_store_entries;
    state_store["hits"] = snapshot.sequence_state_store_hits;
    state_store["misses"] = snapshot.sequence_state_store_misses;
    state_store["evictions"] = snapshot.sequence_state_store_evictions;
    state_store["clone_failures"] = snapshot.sequence_state_store_clone_failures;
    state_store["rejected_device_residency"] = snapshot.sequence_state_store_rejected_device_residency;
    state_store["retained_checkpoint_bytes"] = snapshot.sequence_state_store_retained_checkpoint_bytes;
    state_store["retained_metadata_bytes"] = snapshot.sequence_state_store_retained_metadata_bytes;
    state_store["retained_device_bytes"] = snapshot.sequence_state_store_retained_device_bytes;
    out["sequence_state_store"] = std::move(state_store);
    return json::serialize(out);
}

std::string model_json(const ModelDefinition& model, std::string_view backend) {
    json::object out;
    out["id"] = model.fingerprint().model_id;
    out["format"] = model.fingerprint().format;
    out["architecture"] = model.config().architecture;
    out["layers"] = model.config().layer_count;
    out["embedding"] = model.config().embedding_size;
    out["context_length"] = model.config().context_length;
    out["vocabulary_size"] = model.config().vocabulary_size;
    out["backend"] = backend;
    return json::serialize(out);
}

std::string models_json(const ModelDefinition& model) {
    json::object item;
    item["id"] = model.fingerprint().model_id.empty() ? "air-model" : model.fingerprint().model_id;
    item["object"] = "model";
    item["owned_by"] = "air";

    json::object root;
    root["object"] = "list";
    root["data"] = json::array{std::move(item)};
    return json::serialize(root);
}

std::string metrics_text(const ServiceSnapshot& snapshot) {
    std::string out;
    const auto metric = [&out](std::string_view name, auto value) {
        out += "air_";
        out += name;
        out += ' ';
        out += std::to_string(value);
        out += '\n';
    };
    metric("queued_requests", snapshot.queued_requests);
    metric("active_requests", snapshot.active_requests);
    metric("rejected_overload_requests_total", snapshot.rejected_overload_requests);
    metric("completed_decisions_total", snapshot.completed_decisions);
    metric("max_queued_requests", snapshot.max_queued_requests);
    metric("completed_requests_total", snapshot.completed_requests);
    metric("failed_requests_total", snapshot.failed_requests);
    metric("cancelled_requests_total", snapshot.cancelled_requests);
    metric("stream_delivery_failures_total", snapshot.stream_delivery_failures);
    metric("prompt_tokens_total", snapshot.total_prompt_tokens);
    metric("generated_tokens_total", snapshot.total_generated_tokens);
    metric("prefix_reused_tokens_total", snapshot.total_prefix_reused_tokens);
    metric("peak_kv_bytes", snapshot.peak_kv_bytes);
    metric("peak_device_bytes", snapshot.peak_device_bytes);
    metric("current_kv_bytes", snapshot.current_kv_bytes);
    metric("current_device_bytes", snapshot.current_device_bytes);
    metric("generated_tokens_per_second", snapshot.aggregate_generated_tokens_per_second);
    metric("request_total_ms_p50", snapshot.p50_total_ms);
    metric("request_total_ms_p95", snapshot.p95_total_ms);
    metric("max_prefill_batch_width", snapshot.max_prefill_batch_width);
    metric("max_decode_batch_width", snapshot.max_decode_batch_width);
    metric("stream_queue_capacity", snapshot.stream_queue_capacity);
    metric("admission_reserved_bytes", snapshot.admission_reserved_bytes);
    metric("admission_capacity_bytes", snapshot.admission_capacity_bytes);
    metric("kv_pool_allocated_bytes", snapshot.kv_pool_allocated_bytes);
    metric("kv_pool_free_bytes", snapshot.kv_pool_free_bytes);
    metric("kv_pool_in_use_bytes", snapshot.kv_pool_allocated_bytes >= snapshot.kv_pool_free_bytes
        ? snapshot.kv_pool_allocated_bytes - snapshot.kv_pool_free_bytes : 0U);
    metric("sequence_state_store_entries", snapshot.sequence_state_store_entries);
    metric("sequence_state_store_capacity_entries", snapshot.sequence_state_store_capacity_entries);
    metric("sequence_state_store_hits_total", snapshot.sequence_state_store_hits);
    metric("sequence_state_store_misses_total", snapshot.sequence_state_store_misses);
    metric("sequence_state_store_evictions_total", snapshot.sequence_state_store_evictions);
    metric("sequence_state_store_clone_failures_total", snapshot.sequence_state_store_clone_failures);
    metric("sequence_state_store_rejected_device_residency_total", snapshot.sequence_state_store_rejected_device_residency);
    metric("sequence_state_store_retained_checkpoint_bytes", snapshot.sequence_state_store_retained_checkpoint_bytes);
    metric("sequence_state_store_retained_metadata_bytes", snapshot.sequence_state_store_retained_metadata_bytes);
    metric("sequence_state_store_retained_device_bytes", snapshot.sequence_state_store_retained_device_bytes);
    return out;
}

std::string events_json(const std::vector<RuntimeEvent>& events) {
    json::array array;
    for (const auto& event : events) {
        json::object value;
        value["sequence"] = event.sequence;
        value["unix_ms"] = event.unix_ms;
        value["type"] = event.type;
        value["request_id"] = event.request_id;
        value["detail"] = event.detail;
        array.push_back(std::move(value));
    }
    return json::serialize(array);
}

std::string execution_timeline_json(
    const ExecutionTimelineSnapshot& timeline) {
    json::object root;
    root["schema_version"] = timeline.schema_version;
    root["level"] = json::value(std::string(to_string(timeline.level)));
    root["clock"] = json::value(std::string("steady_clock"));
    root["time_unit"] = json::value(std::string("nanoseconds_from_origin"));
    root["origin_unix_ms"] = timeline.origin_unix_ms;
    root["capacity"] = timeline.capacity;
    root["evicted_spans"] = timeline.evicted_spans;
    root["dropped_spans"] = timeline.dropped_spans;

    json::array spans;
    for (const auto& span : timeline.spans) {
        json::object value;
        value["schema_version"] = span.schema_version;
        value["observation_sequence"] = span.observation_sequence;
        value["request_id"] = span.request_id;
        value["sequence_id"] = span.sequence_id;
        value["scope"] = json::value(std::string(to_string(span.scope)));
        value["category"] = json::value(std::string(to_string(span.category)));
        value["phase"] = json::value(span.phase);
        value["backend"] = json::value(span.backend);
        value["start_ns"] = span.start_ns;
        value["end_ns"] = span.end_ns;
        value["participant_count"] = span.participant_count;
        if (span.work_unit_kind) {
            value["work_unit_kind"] =
                json::value(std::string(to_string(*span.work_unit_kind)));
        } else {
            value["work_unit_kind"] = nullptr;
        }
        value["work_units"] = span.work_units;
        value["success"] = span.success;
        spans.push_back(std::move(value));
    }
    root["spans"] = std::move(spans);
    return json::serialize(root);
}

std::string execution_graph_timeline_json(
    const ExecutionGraphTimelineSnapshot& timeline) {
    json::object root;
    root["schema_version"] = timeline.schema_version;
    root["level"] = json::value(std::string(to_string(timeline.level)));
    root["capacity"] = timeline.capacity;
    root["evicted_graphs"] = timeline.evicted_graphs;
    root["dropped_graphs"] = timeline.dropped_graphs;
    root["derivation_failures"] = timeline.derivation_failures;
    root["topology_status"] = json::value(timeline.topology_status);
    root["topology_fingerprint"] =
        json::value(timeline.topology_fingerprint);
    root["last_derivation_error"] =
        json::value(timeline.last_derivation_error);

    json::array observations;
    for (const auto& observation : timeline.observations) {
        json::object value;
        value["schema_version"] = observation.schema_version;
        value["observation_sequence"] =
            observation.observation_sequence;
        value["start_ns"] = observation.start_ns;
        value["end_ns"] = observation.end_ns;
        value["backend_success"] = observation.backend_success;
        value["evidence_status"] = json::value(
            std::string(to_string(observation.evidence_status)));
        value["evidence_truncated"] =
            observation.evidence_truncated;
        value["planned_transfer_regions"] =
            observation.planned_transfer_regions;
        value["matched_transfer_regions"] =
            observation.matched_transfer_regions;
        value["observed_transfer_spans"] =
            observation.observed_transfer_spans;
        value["unexpected_transfer_spans"] =
            observation.unexpected_transfer_spans;
        value["planned_synchronization_regions"] =
            observation.planned_synchronization_regions;
        value["matched_synchronization_regions"] =
            observation.matched_synchronization_regions;
        value["observed_synchronization_spans"] =
            observation.observed_synchronization_spans;
        value["unexpected_synchronization_spans"] =
            observation.unexpected_synchronization_spans;

        json::array correlations;
        for (const auto& participant : observation.participants) {
            json::object correlation;
            correlation["request_id"] = participant.request_id;
            correlation["sequence_id"] = participant.sequence_id;
            correlations.push_back(std::move(correlation));
        }
        value["participants"] = std::move(correlations);

        if (observation.graph) {
            const auto& graph = *observation.graph;
            json::object graph_value;
            graph_value["schema_version"] = graph.schema_version();
            graph_value["identity"] = json::value(graph.identity());
            graph_value["backend"] =
                json::value(std::string(to_string(graph.backend())));
            graph_value["binding"] =
                json::value(std::string(to_string(graph.binding())));
            graph_value["workload_kind"] =
                json::value(std::string(to_string(graph.workload_kind())));

            json::array invocation_participants;
            std::visit(
                [&](const auto& invocation) {
                    using T = std::decay_t<decltype(invocation)>;
                    if constexpr (std::is_same_v<
                                      T,
                                      AutoregressivePhysicalInvocation>) {
                        graph_value["invocation"] = json::value(
                            std::string(to_string(invocation.kind)));
                        graph_value["topology_fingerprint"] =
                            json::value(invocation.topology_fingerprint);
                        graph_value["hardware_resource_id"] =
                            json::value(invocation.hardware_resource_id);
                        if (invocation.work_unit_kind) {
                            graph_value["work_unit_kind"] = json::value(
                                std::string(to_string(
                                    *invocation.work_unit_kind)));
                        } else {
                            graph_value["work_unit_kind"] = nullptr;
                        }
                        for (const auto& participant :
                             invocation.participants) {
                            json::object item;
                            item["work_units"] = participant.work_units;
                            item["output"] = json::value(
                                std::string(to_string(participant.output)));
                            item["target_count"] =
                                participant.target_count;
                            invocation_participants.push_back(
                                std::move(item));
                        }
                    } else {
                        graph_value["invocation"] =
                            json::value(std::string("iterative-state"));
                        graph_value["topology_fingerprint"] =
                            json::value(invocation.topology_fingerprint);
                        graph_value["hardware_resource_id"] =
                            json::value(invocation.hardware_resource_id);
                        graph_value["work_unit_kind"] =
                            json::value(std::string("iterations"));
                        graph_value["iteration_count"] =
                            invocation.iteration_count;
                        graph_value["active_instances"] =
                            invocation.active_instances;
                    }
                },
                graph.workload_invocation());
            if (graph.autoregressive_state()) {
                graph_value["state_storage"] = json::value(
                    std::string(to_string(
                        graph.autoregressive_state()->storage)));
                if (graph.autoregressive_state()->page_tokens) {
                    graph_value["state_page_tokens"] =
                        *graph.autoregressive_state()->page_tokens;
                }
            } else {
                graph_value["state_storage"] = nullptr;
                graph_value["state_page_tokens"] = nullptr;
            }

            graph_value["participants"] =
                std::move(invocation_participants);

            json::array nodes;
            for (const auto& node : graph.nodes()) {
                json::object node_value;
                node_value["id"] = node.id;
                node_value["kind"] = json::value(
                    std::string(to_string(node.kind)));
                node_value["hardware_resource_id"] =
                    json::value(node.hardware_resource_id);
                node_value["participant_count"] =
                    node.participant_count;
                if (node.work_unit_kind) {
                    node_value["work_unit_kind"] = json::value(
                        std::string(to_string(*node.work_unit_kind)));
                } else {
                    node_value["work_unit_kind"] = nullptr;
                }
                node_value["work_units"] = node.work_units;
                node_value["item_count"] = node.item_count;
                if (node.compute) {
                    node_value["compute"] = json::value(
                        std::string(to_string(*node.compute)));
                }
                if (node.transfer_direction) {
                    node_value["transfer_direction"] = json::value(
                        std::string(to_string(
                            *node.transfer_direction)));
                }
                if (node.payload) {
                    node_value["payload"] = json::value(
                        std::string(to_string(*node.payload)));
                }
                if (node.synchronization) {
                    node_value["synchronization"] = json::value(
                        std::string(to_string(
                            *node.synchronization)));
                }

                json::array dependencies;
                for (const auto dependency : node.dependencies) {
                    dependencies.push_back(dependency);
                }
                node_value["dependencies"] =
                    std::move(dependencies);

                json::array implementations;
                for (const auto& implementation :
                     node.implementations) {
                    json::object binding;
                    binding["site"] = json::value(
                        std::string(to_string(
                            implementation.site)));
                    if (implementation.linear) {
                        binding["family"] =
                            json::value(std::string("linear"));
                        binding["implementation"] = json::value(
                            std::string(to_string(
                                *implementation.linear)));
                    } else if (implementation.attention) {
                        binding["family"] =
                            json::value(std::string("attention"));
                        binding["implementation"] = json::value(
                            std::string(to_string(
                                *implementation.attention)));
                    }
                    implementations.push_back(
                        std::move(binding));
                }
                node_value["implementations"] =
                    std::move(implementations);

                json::array prepared_resources;
                for (const auto& id : node.prepared_resource_ids) {
                    prepared_resources.push_back(json::value(id));
                }
                node_value["prepared_resource_ids"] =
                    std::move(prepared_resources);

                json::array input_values;
                for (const auto& id : node.input_value_ids) {
                    input_values.push_back(json::value(id));
                }
                node_value["input_value_ids"] = std::move(input_values);

                json::array output_values;
                for (const auto& id : node.output_value_ids) {
                    output_values.push_back(json::value(id));
                }
                node_value["output_value_ids"] = std::move(output_values);

                nodes.push_back(std::move(node_value));
            }
            graph_value["nodes"] = std::move(nodes);
            value["graph"] = std::move(graph_value);
        }

        observations.push_back(std::move(value));
    }
    root["observations"] = std::move(observations);
    return json::serialize(root);
}

std::string semantic_registry_json(
    const SemanticRegistrySnapshot& snapshot) {
    json::object root;
    root["schema_version"] = 1U;

    json::array implementations;
    for (const auto& registered : snapshot.implementations) {
        json::object item;
        item["kind"] = json::value(
            std::string(to_string(
                registered.descriptor.requirement.kind)));
        item["semantic_id"] = json::value(
            registered.descriptor.requirement.semantic_id);

        json::object contract_version;
        contract_version["major"] =
            registered.descriptor.requirement.contract_version.major;
        contract_version["minor"] =
            registered.descriptor.requirement.contract_version.minor;
        contract_version["patch"] =
            registered.descriptor.requirement.contract_version.patch;
        item["contract_version"] = std::move(contract_version);

        item["implementation_id"] = json::value(
            registered.descriptor.implementation_id);

        json::object implementation_version;
        implementation_version["major"] =
            registered.descriptor.implementation_version.major;
        implementation_version["minor"] =
            registered.descriptor.implementation_version.minor;
        implementation_version["patch"] =
            registered.descriptor.implementation_version.patch;
        item["implementation_version"] =
            std::move(implementation_version);
        item["origin"] = json::value(
            std::string(to_string(registered.origin)));
        implementations.push_back(std::move(item));
    }
    root["implementations"] = std::move(implementations);

    json::array packages;
    for (const auto& package : snapshot.packages) {
        json::object package_value;
        package_value["package_id"] =
            json::value(package.package_id);
        package_value["resolved_count"] =
            package.resolved_count;
        package_value["missing_count"] =
            package.missing_count;

        json::array requirements;
        for (const auto& status : package.requirements) {
            json::object requirement;
            requirement["kind"] = json::value(
                std::string(to_string(status.requirement.kind)));
            requirement["semantic_id"] = json::value(
                status.requirement.semantic_id);

            json::object version;
            version["major"] =
                status.requirement.contract_version.major;
            version["minor"] =
                status.requirement.contract_version.minor;
            version["patch"] =
                status.requirement.contract_version.patch;
            requirement["contract_version"] = std::move(version);
            requirement["resolved"] = status.resolved();

            if (status.resolved()) {
                requirement["implementation_id"] = json::value(
                    status.implementation->implementation_id);
                requirement["origin"] = json::value(
                    std::string(to_string(*status.origin)));
            } else if (status.missing) {
                requirement["missing_reason"] = json::value(
                    std::string(to_string(status.missing->reason)));
                json::array available_versions;
                for (const auto& available :
                     status.missing->available_versions) {
                    json::object available_value;
                    available_value["major"] = available.major;
                    available_value["minor"] = available.minor;
                    available_value["patch"] = available.patch;
                    available_versions.push_back(
                        std::move(available_value));
                }
                requirement["available_versions"] =
                    std::move(available_versions);
            }
            requirements.push_back(std::move(requirement));
        }
        package_value["requirements"] =
            std::move(requirements);
        packages.push_back(std::move(package_value));
    }
    root["packages"] = std::move(packages);
    return json::serialize(root);
}

} // namespace air::server
