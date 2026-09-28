#pragma once

#include "air/model.hpp"
#include "air/status.hpp"
#include "air/tensor.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace air::detail {

struct PreparedLayerTensorBindings {
    const TensorDescriptor* attention_norm_weight{nullptr};
    const TensorDescriptor* query_weight{nullptr};
    const TensorDescriptor* key_weight{nullptr};
    const TensorDescriptor* value_weight{nullptr};
    const TensorDescriptor* query_bias{nullptr};
    const TensorDescriptor* key_bias{nullptr};
    const TensorDescriptor* value_bias{nullptr};
    const TensorDescriptor* attention_output_weight{nullptr};
    const TensorDescriptor* ffn_norm_weight{nullptr};
    const TensorDescriptor* ffn_gate_weight{nullptr};
    const TensorDescriptor* ffn_up_weight{nullptr};
    const TensorDescriptor* ffn_down_weight{nullptr};
};

struct PreparedModelSemantics {
    // The source ModelDefinition remains canonical truth and must outlive this
    // internal prepared view. Tensor descriptors preserve source identity for
    // provenance while execution consumes semantic roles.
    const ModelDefinition* source_model{nullptr};
    std::string architecture;
    ModelConfig geometry;
    std::uint32_t head_dimension{0};

    const TensorDescriptor* token_embedding_weight{nullptr};
    const TensorDescriptor* output_norm_weight{nullptr};
    const TensorDescriptor* output_weight{nullptr};
    const TensorDescriptor* output_bias{nullptr};
    bool output_weight_tied{false};

    std::vector<PreparedLayerTensorBindings> layers;

    [[nodiscard]] std::vector<const TensorDescriptor*> execution_tensors() const;
};

[[nodiscard]] Status validate_prepared_model_semantics(
    const PreparedModelSemantics& prepared);

} // namespace air::detail
