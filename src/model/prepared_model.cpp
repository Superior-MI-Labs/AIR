#include "model/prepared_model.hpp"

#include <algorithm>
#include <span>
#include <unordered_set>

namespace air::detail {

std::vector<const TensorDescriptor*> PreparedModelSemantics::execution_tensors() const {
    std::vector<const TensorDescriptor*> tensors;
    tensors.reserve(layers.size() * 12U + 4U);

    if (token_embedding_weight) tensors.push_back(token_embedding_weight);
    if (output_norm_weight) tensors.push_back(output_norm_weight);
    if (output_weight && !output_weight_tied) tensors.push_back(output_weight);
    if (output_bias) tensors.push_back(output_bias);

    for (const auto& layer : layers) {
        for (const auto* tensor : {
                 layer.attention_norm_weight,
                 layer.query_weight,
                 layer.key_weight,
                 layer.value_weight,
                 layer.attention_output_weight,
                 layer.ffn_norm_weight,
                 layer.ffn_gate_weight,
                 layer.ffn_up_weight,
                 layer.ffn_down_weight}) {
            if (tensor) tensors.push_back(tensor);
        }
        for (const auto* tensor : {layer.query_bias, layer.key_bias, layer.value_bias}) {
            if (tensor) tensors.push_back(tensor);
        }
    }

    return tensors;
}


namespace {

bool owns_descriptor(const ModelDefinition& model, const TensorDescriptor* descriptor) {
    if (!descriptor) return false;
    return std::any_of(
        model.tensors().begin(), model.tensors().end(),
        [descriptor](const TensorDescriptor& candidate) {
            return &candidate == descriptor;
        });
}

bool same_geometry(const ModelConfig& left, const ModelConfig& right) {
    return left.architecture == right.architecture &&
           left.layer_count == right.layer_count &&
           left.embedding_size == right.embedding_size &&
           left.feed_forward_size == right.feed_forward_size &&
           left.attention_head_count == right.attention_head_count &&
           left.kv_head_count == right.kv_head_count &&
           left.attention_sliding_window == right.attention_sliding_window &&
           left.rope_dimension_count == right.rope_dimension_count &&
           left.context_length == right.context_length &&
           left.vocabulary_size == right.vocabulary_size &&
           left.rope_frequency_base == right.rope_frequency_base &&
           left.rope_scaling_type == right.rope_scaling_type &&
           left.rope_scaling_factor == right.rope_scaling_factor &&
           left.rope_scale_linear == right.rope_scale_linear &&
           left.rms_norm_epsilon == right.rms_norm_epsilon;
}

Status require_shape(const TensorDescriptor* tensor,
                     std::span<const std::uint64_t> expected,
                     const char* role,
                     bool optional = false) {
    if (!tensor) {
        return optional
            ? Status::ok()
            : Status::data_error(std::string("missing prepared semantic role: ") + role);
    }
    if (tensor->shape.dimensions.size() != expected.size()) {
        return Status::data_error(std::string("prepared semantic rank mismatch: ") + role);
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (tensor->shape.dimensions[i] != expected[i]) {
            return Status::data_error(std::string("prepared semantic shape mismatch: ") + role);
        }
    }
    return Status::ok();
}

} // namespace

Status validate_prepared_model_semantics(const PreparedModelSemantics& prepared) {
    if (!prepared.source_model) {
        return Status::invalid_argument("prepared semantic model has no canonical source model");
    }

    const auto canonical = prepared.source_model->validate();
    if (!canonical) return canonical;

    const auto& config = prepared.geometry;
    if (prepared.architecture.empty() ||
        prepared.architecture != config.architecture ||
        config.architecture != prepared.source_model->config().architecture) {
        return Status::invalid_argument("prepared semantic architecture identity is inconsistent");
    }
    if (!same_geometry(config, prepared.source_model->config())) {
        return Status::invalid_argument(
            "prepared semantic geometry diverges from canonical model");
    }
    if (config.layer_count == 0U || config.embedding_size == 0U ||
        config.attention_head_count == 0U || config.kv_head_count == 0U ||
        config.vocabulary_size == 0U || config.context_length == 0U) {
        return Status::data_error("prepared semantic geometry is incomplete");
    }
    if (config.embedding_size % config.attention_head_count != 0U) {
        return Status::data_error("prepared semantic attention geometry is invalid");
    }
    const auto head_dimension =
        config.embedding_size / config.attention_head_count;
    if (prepared.head_dimension != head_dimension) {
        return Status::data_error("prepared semantic head dimension is inconsistent");
    }
    if (config.attention_head_count % config.kv_head_count != 0U) {
        return Status::data_error("prepared semantic KV geometry is invalid");
    }
    if (prepared.layers.size() != config.layer_count) {
        return Status::data_error("prepared semantic layer count is inconsistent");
    }

    const auto owns = [&prepared](const TensorDescriptor* tensor, bool optional = false) {
        if (!tensor) return optional;
        return owns_descriptor(*prepared.source_model, tensor);
    };
    if (!owns(prepared.token_embedding_weight) ||
        !owns(prepared.output_norm_weight) ||
        !owns(prepared.output_weight) ||
        !owns(prepared.output_bias, true)) {
        return Status::invalid_argument(
            "prepared semantic top-level binding does not belong to canonical model");
    }
    if (prepared.output_weight_tied &&
        prepared.output_weight != prepared.token_embedding_weight) {
        return Status::invalid_argument(
            "prepared semantic tied output does not reference token embedding");
    }

    const std::uint64_t embedding = config.embedding_size;
    const std::uint64_t vocab = config.vocabulary_size;
    const std::uint64_t q_width =
        static_cast<std::uint64_t>(config.attention_head_count) * head_dimension;
    const std::uint64_t kv_width =
        static_cast<std::uint64_t>(config.kv_head_count) * head_dimension;
    const std::uint64_t ffn = config.feed_forward_size;
    if (ffn == 0U) {
        return Status::data_error("prepared semantic feed-forward geometry is missing");
    }

    const std::uint64_t embedding_shape[] = {embedding, vocab};
    const std::uint64_t norm_shape[] = {embedding};
    const std::uint64_t output_shape[] = {embedding, vocab};
    const std::uint64_t output_bias_shape[] = {vocab};

    auto status = require_shape(
        prepared.token_embedding_weight, embedding_shape, "token_embedding_weight");
    if (!status) return status;
    status = require_shape(
        prepared.output_norm_weight, norm_shape, "output_norm_weight");
    if (!status) return status;
    status = require_shape(
        prepared.output_weight, output_shape, "output_weight");
    if (!status) return status;
    status = require_shape(
        prepared.output_bias, output_bias_shape, "output_bias", true);
    if (!status) return status;

    for (std::size_t index = 0; index < prepared.layers.size(); ++index) {
        const auto& layer = prepared.layers[index];
        for (const auto* tensor : {
                 layer.attention_norm_weight,
                 layer.query_weight,
                 layer.key_weight,
                 layer.value_weight,
                 layer.attention_output_weight,
                 layer.ffn_norm_weight,
                 layer.ffn_gate_weight,
                 layer.ffn_up_weight,
                 layer.ffn_down_weight}) {
            if (!owns(tensor)) {
                return Status::invalid_argument(
                    "prepared semantic required layer binding does not belong to canonical model");
            }
        }
        for (const auto* tensor : {
                 layer.query_bias, layer.key_bias, layer.value_bias}) {
            if (!owns(tensor, true)) {
                return Status::invalid_argument(
                    "prepared semantic optional layer binding does not belong to canonical model");
            }
        }

        const std::uint64_t q_shape[] = {embedding, q_width};
        const std::uint64_t kv_shape[] = {embedding, kv_width};
        const std::uint64_t q_bias_shape[] = {q_width};
        const std::uint64_t kv_bias_shape[] = {kv_width};
        const std::uint64_t projection_shape[] = {q_width, embedding};
        const std::uint64_t ffn_in_shape[] = {embedding, ffn};
        const std::uint64_t ffn_out_shape[] = {ffn, embedding};

        status = require_shape(
            layer.attention_norm_weight, norm_shape, "layer.attention_norm_weight");
        if (!status) return status;
        status = require_shape(layer.query_weight, q_shape, "layer.query_weight");
        if (!status) return status;
        status = require_shape(layer.key_weight, kv_shape, "layer.key_weight");
        if (!status) return status;
        status = require_shape(layer.value_weight, kv_shape, "layer.value_weight");
        if (!status) return status;
        status = require_shape(
            layer.query_bias, q_bias_shape, "layer.query_bias", true);
        if (!status) return status;
        status = require_shape(
            layer.key_bias, kv_bias_shape, "layer.key_bias", true);
        if (!status) return status;
        status = require_shape(
            layer.value_bias, kv_bias_shape, "layer.value_bias", true);
        if (!status) return status;
        status = require_shape(
            layer.attention_output_weight, projection_shape,
            "layer.attention_output_weight");
        if (!status) return status;
        status = require_shape(
            layer.ffn_norm_weight, norm_shape, "layer.ffn_norm_weight");
        if (!status) return status;
        status = require_shape(
            layer.ffn_gate_weight, ffn_in_shape, "layer.ffn_gate_weight");
        if (!status) return status;
        status = require_shape(
            layer.ffn_up_weight, ffn_in_shape, "layer.ffn_up_weight");
        if (!status) return status;
        status = require_shape(
            layer.ffn_down_weight, ffn_out_shape, "layer.ffn_down_weight");
        if (!status) return status;
    }

    std::unordered_set<const TensorDescriptor*> unique_execution_tensors;
    for (const auto* tensor : prepared.execution_tensors()) {
        if (!tensor) {
            return Status::invalid_argument(
                "prepared semantic execution tensor set contains null");
        }
        if (!unique_execution_tensors.insert(tensor).second) {
            return Status::invalid_argument(
                "prepared semantic roles duplicate one physical tensor descriptor");
        }
    }

    return Status::ok();
}

} // namespace air::detail
