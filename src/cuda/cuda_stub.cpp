#include "air/cuda.hpp"
#include "air/machine.hpp"

namespace air {

bool cuda_compiled() noexcept { return false; }

Result<std::vector<DeviceInfo>> cuda_devices() {
    return Status::unsupported("AIR was built without CUDA support");
}

Status augment_hardware_topology_with_cuda(HardwareTopology& topology) {
    topology.fingerprint = hardware_topology_fingerprint(topology);
    return Status::ok();
}

Status augment_hardware_environment_with_cuda(
    const HardwareTopology& topology,
    HardwareEnvironmentSnapshot& environment) {
    if (environment.topology_fingerprint != topology.fingerprint) {
        return Status::invalid_argument(
            "CUDA-disabled environment requires matching topology fingerprint");
    }
    return Status::ok();
}

Status augment_hardware_discovery_with_cuda(HardwareDiscovery& discovery) {
    auto status = augment_hardware_topology_with_cuda(discovery.topology);
    if (!status) return status;
    discovery.environment.topology_fingerprint = discovery.topology.fingerprint;
    return augment_hardware_environment_with_cuda(
        discovery.topology, discovery.environment);
}

Result<HardwareTopology> discover_machine_topology() {
    auto topology = discover_host_topology();
    if (!topology) return topology.status();

    auto status = augment_hardware_topology_with_cuda(topology.value());
    if (!status) return status;
    return std::move(topology).value();
}

Result<HardwareEnvironmentSnapshot> observe_machine_environment(
    const HardwareTopology& topology) {
    auto environment = observe_host_environment(topology);
    if (!environment) return environment.status();

    auto status =
        augment_hardware_environment_with_cuda(topology, environment.value());
    if (!status) return status;
    return std::move(environment).value();
}

Result<HardwareDiscovery> discover_machine_hardware() {
    auto topology = discover_machine_topology();
    if (!topology) return topology.status();

    auto environment = observe_machine_environment(topology.value());
    if (!environment) return environment.status();

    return HardwareDiscovery{
        std::move(topology).value(),
        std::move(environment).value(),
    };
}

struct CudaKvCache::Impl {
    ExecutionCorrelation execution_correlation{};
};
CudaKvCache::CudaKvCache(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
CudaKvCache::~CudaKvCache() = default;
CudaKvCache::CudaKvCache(CudaKvCache&&) noexcept = default;
CudaKvCache& CudaKvCache::operator=(CudaKvCache&&) noexcept = default;
std::uint64_t CudaKvCache::size() const noexcept { return 0U; }
std::uint64_t CudaKvCache::capacity() const noexcept { return 0U; }
std::uint32_t CudaKvCache::page_tokens() const noexcept { return 0U; }
std::uint64_t CudaKvCache::committed_bytes() const noexcept { return 0U; }
std::uint64_t CudaKvCache::resident_bytes() const noexcept { return 0U; }
void CudaKvCache::bind_execution_correlation(ExecutionCorrelation correlation) noexcept {
    if (impl_) impl_->execution_correlation = correlation;
}
ExecutionCorrelation CudaKvCache::execution_correlation() const noexcept {
    return impl_ ? impl_->execution_correlation : ExecutionCorrelation{};
}
Result<std::unique_ptr<CudaKvCache>> CudaKvCache::fork(std::uint64_t) const {
    return Status::unsupported("AIR was built without CUDA support");
}
void CudaKvCache::reset() noexcept {}

struct CudaExecutor::Impl {};
CudaExecutor::CudaExecutor(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
CudaExecutor::~CudaExecutor() = default;
CudaExecutor::CudaExecutor(CudaExecutor&&) noexcept = default;
CudaExecutor& CudaExecutor::operator=(CudaExecutor&&) noexcept = default;
Result<std::unique_ptr<CudaExecutor>> CudaExecutor::create(std::shared_ptr<const ModelDefinition>, int) {
    return Status::unsupported("AIR was built without CUDA support");
}
const ModelDefinition& CudaExecutor::model() const noexcept { std::terminate(); }
int CudaExecutor::device_ordinal() const noexcept { return -1; }
CudaExecutionStats CudaExecutor::stats() const noexcept { return {}; }
Status CudaExecutor::prepare_linear_tactic(QuantizedLinearExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
std::uint64_t CudaExecutor::estimate_linear_tactic_preparation_bytes(QuantizedLinearExecutionKind) const noexcept {
    return 0U;
}
Status CudaExecutor::trim_linear_tactics(std::span<const QuantizedLinearExecutionKind>) {
    return Status::ok();
}
Result<std::unique_ptr<CudaKvCache>> CudaExecutor::create_kv_cache(std::uint32_t) const {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<float>> CudaExecutor::step(TokenId, CudaKvCache&, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<float>> CudaExecutor::step(TokenId, CudaKvCache&, QuantizedLinearExecutionKind, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<float>> CudaExecutor::step_verified(TokenId, CudaKvCache&, VerificationTrace&, QuantizedLinearExecutionKind, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<TokenId> CudaExecutor::step_greedy(TokenId, CudaKvCache&, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<TokenId> CudaExecutor::step_greedy(TokenId, CudaKvCache&, QuantizedLinearExecutionKind, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<float>> CudaExecutor::step_target_logprobs(
    TokenId, std::span<const TokenId>, CudaKvCache&,
    QuantizedLinearExecutionKind, QuantizedLinearExecutionKind,
    AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<TokenId>> CudaExecutor::step_greedy_batch(std::span<const TokenId>, std::span<CudaKvCache*>, QuantizedLinearExecutionKind, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<float>> CudaExecutor::prefill(std::span<const TokenId>, CudaKvCache&, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Status CudaExecutor::prefill_discard(std::span<const TokenId>, CudaKvCache&, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<TokenId> CudaExecutor::prefill_greedy(std::span<const TokenId>, CudaKvCache&, QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<std::vector<float>> CudaExecutor::prefill_target_logprobs(
    std::span<const TokenId>, std::span<const TokenId>, CudaKvCache&,
    QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<CudaPrefillBatchExecution> CudaExecutor::prefill_batch(
    std::span<const CudaPrefillBatchItem>,
    QuantizedLinearExecutionKind, AttentionExecutionKind) {
    return Status::unsupported("AIR was built without CUDA support");
}
Result<GenerationResult> CudaExecutor::generate(std::span<const TokenId>, const GenerationConfig&) {
    return Status::unsupported("AIR was built without CUDA support");
}

} // namespace air
