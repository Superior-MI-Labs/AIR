#pragma once

#include "air/model.hpp"
#include "air/result.hpp"
#include "air/status.hpp"
#include "model/prepared_model.hpp"

#include <string_view>

namespace air::detail {

class ModelArchitectureAdapter {
public:
    virtual ~ModelArchitectureAdapter() = default;

    [[nodiscard]] virtual std::string_view architecture() const noexcept = 0;
    [[nodiscard]] virtual Result<PreparedModelSemantics> prepare(
        const ModelDefinition& model) const = 0;
};

[[nodiscard]] Result<const ModelArchitectureAdapter*> resolve_model_architecture(
    const ModelDefinition& model);

} // namespace air::detail
