#pragma once

#include "air/model.hpp"
#include "air/result.hpp"
#include "air/status.hpp"
#include "model/prepared_model.hpp"

#include <string>
#include <vector>

namespace air::detail {

[[nodiscard]] Status validate_qwen2_structure(const ModelDefinition& model);
[[nodiscard]] Result<PreparedModelSemantics> prepare_qwen2_semantics(const ModelDefinition& model);
[[nodiscard]] std::vector<std::string> qwen2_execution_tensor_names(const ModelDefinition& model);

} // namespace air::detail
