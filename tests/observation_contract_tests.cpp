#include "runtime/backend.hpp"

#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

class FakeCheckpoint final : public air::runtime_detail::SequenceCheckpoint {
public:
    [[nodiscard]] air::BackendKind backend() const noexcept override {
        return air::BackendKind::reference;
    }
    [[nodiscard]] std::uint64_t tokens() const noexcept override { return 0U; }
    [[nodiscard]] air::runtime_detail::SequenceResources resources() const noexcept override {
        return {};
    }
    [[nodiscard]] air::Result<std::unique_ptr<air::runtime_detail::SequenceCheckpoint>>
    clone() const override {
        return std::unique_ptr<air::runtime_detail::SequenceCheckpoint>(
            std::make_unique<FakeCheckpoint>());
    }
};

class FakeSequence final : public air::runtime_detail::SequenceState {
public:
    void bind_execution_correlation(air::ExecutionCorrelation correlation) noexcept override {
        air::runtime_detail::SequenceState::bind_execution_correlation(correlation);
        ++bind_count_;
    }

    [[nodiscard]] std::uint32_t bind_count() const noexcept { return bind_count_; }

    [[nodiscard]] air::BackendKind backend() const noexcept override {
        return air::BackendKind::reference;
    }
    [[nodiscard]] air::Result<std::vector<float>>
    prefill(std::span<const air::TokenId>) override {
        return std::vector<float>{0.0F};
    }
    [[nodiscard]] air::Result<std::vector<float>>
    decode(air::TokenId) override {
        return std::vector<float>{0.0F};
    }
    [[nodiscard]] air::runtime_detail::SequenceResources resources() const noexcept override {
        return {};
    }
    [[nodiscard]] air::Result<std::unique_ptr<air::runtime_detail::SequenceCheckpoint>>
    checkpoint() const override {
        return std::unique_ptr<air::runtime_detail::SequenceCheckpoint>(
            std::make_unique<FakeCheckpoint>());
    }

private:
    std::uint32_t bind_count_{0};
};

class FakePreparedModel final : public air::runtime_detail::PreparedModel {
public:
    FakePreparedModel() {
        capabilities_.backend = air::BackendKind::reference;
        capabilities_.prefill_execution = air::PrefillExecutionKind::serial;
        capabilities_.kv_storage = air::KvStorageKind::paged;
        capabilities_.max_prefill_batch_width = 1U;
        capabilities_.max_decode_batch_width = 1U;
        capabilities_.sequence_checkpointing = true;
    }

    [[nodiscard]] air::BackendKind backend() const noexcept override {
        return air::BackendKind::reference;
    }
    [[nodiscard]] const air::BackendCapabilities& capabilities() const noexcept override {
        return capabilities_;
    }
    [[nodiscard]] const air::ModelDefinition& model() const noexcept override {
        return model_;
    }

    [[nodiscard]] air::Result<std::unique_ptr<air::runtime_detail::SequenceState>>
    create_sequence(const air::ExecutionPlan&) override {
        return std::unique_ptr<air::runtime_detail::SequenceState>(
            std::make_unique<FakeSequence>());
    }

    [[nodiscard]] air::Result<std::unique_ptr<air::runtime_detail::SequenceState>>
    restore_sequence(
        const air::ExecutionPlan&,
        const air::runtime_detail::SequenceCheckpoint&) override {
        return std::unique_ptr<air::runtime_detail::SequenceState>(
            std::make_unique<FakeSequence>());
    }

    [[nodiscard]] std::uint64_t resident_device_bytes() const noexcept override {
        return 0U;
    }
    [[nodiscard]] std::uint64_t estimate_sequence_device_bytes(
        const air::ExecutionPlan&, std::uint64_t) const noexcept override {
        return 0U;
    }
    [[nodiscard]] std::optional<std::uint64_t> sequence_capacity_bytes() const override {
        return std::nullopt;
    }
    [[nodiscard]] std::optional<std::uint64_t> free_device_bytes() const override {
        return std::nullopt;
    }
    [[nodiscard]] std::uint64_t kv_pool_allocated_bytes() const noexcept override {
        return 0U;
    }
    [[nodiscard]] std::uint64_t kv_pool_free_bytes() const noexcept override {
        return 0U;
    }

private:
    air::BackendCapabilities capabilities_{};
    air::ModelDefinition model_{};
};

void test_additive_correlation_wrappers() {
    FakePreparedModel concrete;
    air::runtime_detail::PreparedModel& prepared = concrete;

    air::ExecutionPlan plan;
    plan.backend = air::BackendKind::reference;
    plan.strategy_id = "observation-contract";
    plan.scheduling.prefill_quantum_tokens = 1U;
    plan.kv.page_tokens = 1U;

    air::ExecutionCorrelation initial;
    initial.request_id = 41U;
    initial.sequence_id = 73U;

    auto created = prepared.create_sequence(plan, initial);
    check(created.is_ok(), "correlation-aware create delegates to existing backend contract");
    if (!created) return;

    const auto& created_correlation = created.value()->execution_correlation();
    check(created_correlation.request_id == 41U &&
          created_correlation.sequence_id == 73U &&
          created_correlation.sink == nullptr,
          "create wrapper binds exact request/sequence correlation");

    const auto* created_fake =
        dynamic_cast<const FakeSequence*>(created.value().get());
    check(created_fake != nullptr && created_fake->bind_count() == 1U,
          "create wrapper dispatches through the derived correlation-binding hook");

    auto checkpoint = created.value()->checkpoint();
    check(checkpoint.is_ok(), "fake correlated sequence checkpoints");
    if (!checkpoint) return;

    air::ExecutionCorrelation restored;
    restored.request_id = 42U;
    restored.sequence_id = 74U;

    auto sequence = prepared.restore_sequence(plan, *checkpoint.value(), restored);
    check(sequence.is_ok(), "correlation-aware restore delegates to existing backend contract");
    if (!sequence) return;

    const auto& restored_correlation = sequence.value()->execution_correlation();
    check(restored_correlation.request_id == 42U &&
          restored_correlation.sequence_id == 74U &&
          restored_correlation.sink == nullptr,
          "restore wrapper binds caller-provided correlation");

    const auto* restored_fake =
        dynamic_cast<const FakeSequence*>(sequence.value().get());
    check(restored_fake != nullptr && restored_fake->bind_count() == 1U,
          "restore wrapper dispatches through the derived correlation-binding hook");
}

} // namespace

int main() {
    test_additive_correlation_wrappers();

    if (failures != 0) {
        std::cerr << failures << " observation contract test(s) failed\n";
        return 1;
    }
    std::cout << "observation contract tests passed\n";
    return 0;
}
