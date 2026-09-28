#include "model/architecture_adapter.hpp"
#include "model/qwen2_contract.hpp"

namespace air::detail {
namespace {

class Qwen2ArchitectureAdapter final : public ModelArchitectureAdapter {
public:
    [[nodiscard]] std::string_view architecture() const noexcept override {
        return "qwen2";
    }

    [[nodiscard]] Status validate(const ModelDefinition& model) const override {
        return validate_qwen2_structure(model);
    }

    [[nodiscard]] Result<PreparedModelSemantics> prepare(
        const ModelDefinition& model) const override {
        return prepare_qwen2_semantics(model);
    }

    [[nodiscard]] std::vector<std::string> execution_tensor_names(
        const ModelDefinition& model) const override {
        return qwen2_execution_tensor_names(model);
    }
};

const Qwen2ArchitectureAdapter qwen2_adapter{};

} // namespace

Result<const ModelArchitectureAdapter*> resolve_model_architecture(
    const ModelDefinition& model) {
    if (model.config().architecture == qwen2_adapter.architecture()) {
        return static_cast<const ModelArchitectureAdapter*>(&qwen2_adapter);
    }

    return Status::unsupported(
        "AIR has no model architecture adapter for: " + model.config().architecture);
}

} // namespace air::detail
