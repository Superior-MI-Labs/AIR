#pragma once

#include "air/result.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace air {

enum class BackendKind {
    reference = 0,
    cuda,
};

[[nodiscard]] const char* to_string(BackendKind backend) noexcept;
[[nodiscard]] Result<BackendKind> backend_kind_from_string(std::string_view value);

enum class PrefillExecutionKind {
    serial = 0,
    native_batch,
};

[[nodiscard]] const char* to_string(PrefillExecutionKind kind) noexcept;

enum class KvStorageKind {
    contiguous = 0,
    paged,
};

[[nodiscard]] const char* to_string(KvStorageKind kind) noexcept;

enum class QuantizedLinearExecutionKind {
    baseline = 0,
    batch_reuse4,
    batch_reuse8,
    q5q8_dp4a_hybrid,
    dense_f32_cublas,
};

[[nodiscard]] const char* to_string(QuantizedLinearExecutionKind kind) noexcept;
[[nodiscard]] Result<QuantizedLinearExecutionKind> quantized_linear_execution_kind_from_string(
    std::string_view value);

enum class AttentionExecutionKind {
    baseline = 0,
    online_softmax,
};

[[nodiscard]] const char* to_string(AttentionExecutionKind kind) noexcept;
[[nodiscard]] Result<AttentionExecutionKind> attention_execution_kind_from_string(
    std::string_view value);

// QualifiedOperationSite is intentionally limited to operation sites that are
// already selectable in the qualified AIR Qwen execution path. It is not a
// universal neural-operation catalog.
enum class QualifiedOperationSite {
    prefill_transformer_block_linear = 0,
    decode_transformer_block_linear,
    decode_output_projection,
    prefill_attention,
    decode_attention,
};

enum class OperationImplementationFamily {
    linear = 0,
    attention,
};

[[nodiscard]] const char* to_string(QualifiedOperationSite site) noexcept;
[[nodiscard]] const char* to_string(OperationImplementationFamily family) noexcept;
[[nodiscard]] Result<OperationImplementationFamily> implementation_family(
    QualifiedOperationSite site);

struct BackendCapabilities {
    BackendKind backend{BackendKind::reference};
    PrefillExecutionKind prefill_execution{PrefillExecutionKind::serial};
    KvStorageKind kv_storage{KvStorageKind::contiguous};
    std::uint32_t max_prefill_batch_width{1};
    std::uint32_t max_decode_batch_width{1};
    bool sequence_checkpointing{false};
    bool exact_prefix_reuse{false};
    // True only when the backend can implement temperature<=0 token selection
    // without materializing the full vocabulary logits on the host. The host
    // Sampler remains the semantic oracle for all stochastic policies.
    bool device_greedy_selection{false};
    // Linear tactics are capability-scoped by operation. Transformer-block
    // matrices and the terminal vocabulary projection are not assumed to share
    // the same valid tactic set. This prevents a broad phase-level tactic from
    // silently leaking into an operation it was never prepared to execute.
    std::vector<QuantizedLinearExecutionKind> prefill_block_quantized_linear{QuantizedLinearExecutionKind::baseline};
    std::vector<QuantizedLinearExecutionKind> decode_block_quantized_linear{QuantizedLinearExecutionKind::baseline};
    std::vector<QuantizedLinearExecutionKind> decode_output_quantized_linear{QuantizedLinearExecutionKind::baseline};
    std::vector<AttentionExecutionKind> prefill_attention{AttentionExecutionKind::baseline};
    std::vector<AttentionExecutionKind> decode_attention{AttentionExecutionKind::baseline};
};

[[nodiscard]] Result<std::span<const QuantizedLinearExecutionKind>>
linear_implementations(
    const BackendCapabilities& capabilities,
    QualifiedOperationSite site);

[[nodiscard]] Result<std::span<const AttentionExecutionKind>>
attention_implementations(
    const BackendCapabilities& capabilities,
    QualifiedOperationSite site);

struct RequestProfile {
    std::uint64_t prompt_tokens{0};
    std::uint64_t max_output_tokens{0};
    std::uint32_t active_sequences{1};
};

struct RuntimeSnapshot {
    std::uint64_t free_device_memory_bytes{0};
    std::uint64_t resident_kv_bytes{0};
    std::uint64_t prepared_artifact_bytes{0};
    std::string current_strategy_id;
    double device_utilization{0.0};
};

// SchedulingPolicy contains scheduler behavior, not backend-kernel geometry.
// prefill_quantum_tokens is the maximum prompt work a sequence may consume
// before yielding back to the micro-scheduler. It does not imply native
// multi-token GPU prefill.
struct SchedulingPolicy {
    std::uint32_t prefill_quantum_tokens{32};
};

// KvPolicy describes physical KV geometry only. page_tokens must be present
// only for backends whose KV storage is physically paged.
struct KvPolicy {
    std::optional<std::uint32_t> page_tokens;
};

struct LinearPolicy {
    // Transformer-block linear work during native prefill.
    QuantizedLinearExecutionKind prefill_block{QuantizedLinearExecutionKind::baseline};
    // Transformer-block linear work during decode.
    QuantizedLinearExecutionKind decode_block{QuantizedLinearExecutionKind::baseline};
    // Terminal vocabulary projection for native multi-sequence decode. Keeping
    // this explicit avoids hidden tensor-name fallback when a block tactic has
    // a narrower preparation scope (for example dense-f32-cublas).
    QuantizedLinearExecutionKind decode_output{QuantizedLinearExecutionKind::baseline};
};

struct AttentionPolicy {
    AttentionExecutionKind prefill{AttentionExecutionKind::baseline};
    AttentionExecutionKind decode{AttentionExecutionKind::baseline};
};

struct ExecutionPlan {
    BackendKind backend{BackendKind::reference};
    std::string strategy_id{"static"};
    SchedulingPolicy scheduling{};
    KvPolicy kv{};
    LinearPolicy linear{};
    AttentionPolicy attention{};
};

[[nodiscard]] Status validate_execution_plan(const ExecutionPlan& plan,
                                             const BackendCapabilities& capabilities);

// ExecutionGraph R0 describes one already-concrete physical backend invocation.
// It is derived execution data, not model semantics, scheduler authority, or an
// executable graph. The production backend path remains unchanged in Prompt 5B.
inline constexpr std::uint32_t execution_graph_schema_version = 1U;

enum class PhysicalInvocationKind {
    prefill_single = 0,
    prefill_native_batch,
    decode_single,
    decode_native_greedy_batch,
};

enum class PhysicalOutputMode {
    discard = 0,
    logits,
    greedy,
    target_logprobs,
};

enum class ExecutionGraphNodeKind {
    compute_region = 0,
    transfer_region,
    synchronization_region,
};

enum class ExecutionComputeRegionKind {
    model = 0,
    device_greedy_selection,
    target_logprob_reduction,
};

enum class ExecutionTransferDirection {
    host_to_device = 0,
    device_to_host,
};

enum class ExecutionPayloadKind {
    input_tokens = 0,
    full_logits,
    greedy_result,
    target_tokens,
    target_logprob_results,
};

enum class ExecutionSynchronizationKind {
    backend_stream_wait = 0,
};

[[nodiscard]] const char* to_string(PhysicalInvocationKind kind) noexcept;
[[nodiscard]] const char* to_string(PhysicalOutputMode mode) noexcept;
[[nodiscard]] const char* to_string(ExecutionGraphNodeKind kind) noexcept;
[[nodiscard]] const char* to_string(ExecutionComputeRegionKind kind) noexcept;
[[nodiscard]] const char* to_string(ExecutionTransferDirection direction) noexcept;
[[nodiscard]] const char* to_string(ExecutionPayloadKind payload) noexcept;
[[nodiscard]] const char* to_string(ExecutionSynchronizationKind kind) noexcept;

struct PhysicalInvocationParticipant {
    // Amount of scheduler/backend work represented by this participant. For
    // token paths this is the concrete token count of the invocation.
    std::uint64_t work_units{0};

    // Backend output behavior already selected by the serving path.
    PhysicalOutputMode output{PhysicalOutputMode::logits};

    // Non-zero only for target-logprob output.
    std::uint32_t target_count{0};
};

struct PhysicalInvocation {
    PhysicalInvocationKind kind{PhysicalInvocationKind::prefill_single};

    // Physical ordering is retained because native backend packing may depend on
    // participant order. Request/sequence IDs deliberately do not appear here.
    std::vector<PhysicalInvocationParticipant> participants;

    // Existing HardwareTopology node/resource identity chosen by the current
    // backend/placement authority. ExecutionGraph references it but never owns
    // or rediscovers topology.
    std::string hardware_resource_id;
};

struct ExecutionImplementationBinding {
    QualifiedOperationSite site{QualifiedOperationSite::prefill_transformer_block_linear};
    std::optional<QuantizedLinearExecutionKind> linear;
    std::optional<AttentionExecutionKind> attention;
};

struct ExecutionGraphNode {
    std::uint32_t id{0};
    ExecutionGraphNodeKind kind{ExecutionGraphNodeKind::compute_region};
    std::string hardware_resource_id;
    std::uint32_t participant_count{0};
    std::uint64_t work_units{0};
    std::vector<std::uint32_t> dependencies;

    std::optional<ExecutionComputeRegionKind> compute;
    std::optional<ExecutionTransferDirection> transfer_direction;
    std::optional<ExecutionPayloadKind> payload;
    std::optional<ExecutionSynchronizationKind> synchronization;

    // Present only on the model compute region. These are references to the
    // already-selected Prompt 4 implementation identities, not a new registry.
    std::vector<ExecutionImplementationBinding> implementations;
};

class ExecutionGraph final {
public:
    ExecutionGraph(std::string identity,
                   BackendKind backend,
                   PhysicalInvocation invocation,
                   KvStorageKind state_storage,
                   std::optional<std::uint32_t> state_page_tokens,
                   std::vector<ExecutionGraphNode> nodes)
        : identity_(std::move(identity)),
          backend_(backend),
          invocation_(std::move(invocation)),
          state_storage_(state_storage),
          state_page_tokens_(state_page_tokens),
          nodes_(std::move(nodes)) {}

    [[nodiscard]] std::uint32_t schema_version() const noexcept {
        return execution_graph_schema_version;
    }
    [[nodiscard]] const std::string& identity() const noexcept { return identity_; }
    [[nodiscard]] BackendKind backend() const noexcept { return backend_; }
    [[nodiscard]] const PhysicalInvocation& invocation() const noexcept {
        return invocation_;
    }
    [[nodiscard]] KvStorageKind state_storage() const noexcept {
        return state_storage_;
    }
    [[nodiscard]] const std::optional<std::uint32_t>& state_page_tokens() const noexcept {
        return state_page_tokens_;
    }
    [[nodiscard]] std::span<const ExecutionGraphNode> nodes() const noexcept {
        return nodes_;
    }

private:
    std::string identity_;
    BackendKind backend_{BackendKind::reference};
    PhysicalInvocation invocation_{};
    KvStorageKind state_storage_{KvStorageKind::contiguous};
    std::optional<std::uint32_t> state_page_tokens_;
    std::vector<ExecutionGraphNode> nodes_;
};

// Pure projection only. This function validates the existing plan against the
// existing capability authority and derives physical description data. It does
// not schedule, allocate, prepare, restore, execute, or observe a backend.
[[nodiscard]] Result<ExecutionGraph> derive_execution_graph(
    const ExecutionPlan& plan,
    const BackendCapabilities& capabilities,
    PhysicalInvocation invocation);

// Stable inspection form. Identity is a digest of the same canonical structural
// fields excluding the identity line itself.
[[nodiscard]] std::string serialize_execution_graph(const ExecutionGraph& graph);


} // namespace air
