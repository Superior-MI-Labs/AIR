#pragma once

#include "air/cuda.hpp"
#include "model/prepared_model.hpp"

namespace air::detail {

class CudaExecutorFactory final {
public:
    [[nodiscard]] static Result<std::unique_ptr<CudaExecutor>> create(
        std::shared_ptr<const ModelDefinition> model,
        int device_ordinal,
        PreparedModelSemantics prepared);
};

} // namespace air::detail
