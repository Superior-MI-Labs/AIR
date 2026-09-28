#pragma once

#include "air/reference.hpp"
#include "model/prepared_model.hpp"

namespace air::detail {

class ReferenceExecutorFactory final {
public:
    [[nodiscard]] static Result<std::unique_ptr<ReferenceExecutor>> create(
        std::shared_ptr<const ModelDefinition> model,
        PreparedModelSemantics prepared);
};

} // namespace air::detail
