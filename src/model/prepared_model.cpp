#include "model/prepared_model.hpp"

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

} // namespace air::detail
