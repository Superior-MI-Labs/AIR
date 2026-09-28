#pragma once

#include "air/model.hpp"
#include "air/result.hpp"
#include "air/status.hpp"
#include "model/prepared_model.hpp"

namespace air::detail {

[[nodiscard]] Result<PreparedModelSemantics> prepare_qwen2_semantics(
    const ModelDefinition& model);

} // namespace air::detail
