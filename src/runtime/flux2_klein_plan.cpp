#include "air/flux2_klein_plan.hpp"

#include <algorithm>
#include <array>
#include <unordered_set>
#include <utility>

namespace air {
namespace {

bool contains_value_id(
    const Flux2KleinSemanticContract& semantics,
    const std::string& id) {
    const std::array<const Flux2KleinSemanticValueIdentity*, 9> values{
        &semantics.values.prompt_text,
        &semantics.values.positive_conditioning,
        &semantics.values.negative_conditioning,
        &semantics.values.seed,
        &semantics.values.noise,
        &semantics.values.sigma_schedule,
        &semantics.values.initial_latent,
        &semantics.values.sampled_latent,
        &semantics.values.decoded_image,
    };
    return std::any_of(
        values.begin(), values.end(),
        [&](const auto* value) { return value->id == id; });
}

Status validate_value_ids(
    const Flux2KleinSemanticContract& semantics,
    const Flux2KleinPhysicalPhase& phase) {
    std::unordered_set<std::string> outputs;
    for (const auto& id : phase.input_value_ids) {
        if (id.empty() || !contains_value_id(semantics, id)) {
            return Status::invalid_argument(
                "FLUX.2 physical phase references unknown semantic input identity");
        }
    }
    for (const auto& id : phase.output_value_ids) {
        if (id.empty() || !contains_value_id(semantics, id)) {
            return Status::invalid_argument(
                "FLUX.2 physical phase references unknown semantic output identity");
        }
        if (!outputs.insert(id).second) {
            return Status::invalid_argument(
                "FLUX.2 physical phase output identities must be unique");
        }
    }
    return Status::ok();
}

} // namespace

const char* to_string(Flux2KleinPhysicalPhaseKind kind) noexcept {
    switch (kind) {
    case Flux2KleinPhysicalPhaseKind::conditioning:
        return "conditioning";
    case Flux2KleinPhysicalPhaseKind::iterative_denoise:
        return "iterative-denoise";
    case Flux2KleinPhysicalPhaseKind::decode_image:
        return "decode-image";
    }
    return "unknown";
}

Result<Flux2KleinPhysicalPlan> make_flux2_klein_component_plan(
    const Flux2KleinSemanticContract& semantics,
    std::string topology_fingerprint,
    std::string hardware_resource_id) {
    const auto semantic_status =
        validate_flux2_klein_oracle_semantics(semantics);
    if (!semantic_status) return semantic_status;
    if (topology_fingerprint.empty()) {
        return Status::invalid_argument(
            "FLUX.2 component plan requires topology identity");
    }
    if (hardware_resource_id.empty()) {
        return Status::invalid_argument(
            "FLUX.2 component plan requires hardware resource identity");
    }

    Flux2KleinPhysicalPlan plan;

    Flux2KleinPhysicalPhase conditioning;
    conditioning.phase_id = 0U;
    conditioning.kind = Flux2KleinPhysicalPhaseKind::conditioning;
    conditioning.topology_fingerprint = topology_fingerprint;
    conditioning.hardware_resource_id = hardware_resource_id;
    conditioning.prepared_resources.push_back(
        PreparedResourceRequirement{
            std::string(flux2_klein_text_encoder_resource_id),
            std::nullopt});
    conditioning.input_value_ids = {
        semantics.values.prompt_text.id,
    };
    conditioning.output_value_ids = {
        semantics.values.positive_conditioning.id,
        semantics.values.negative_conditioning.id,
    };
    plan.phases.push_back(std::move(conditioning));

    Flux2KleinPhysicalPhase denoise;
    denoise.phase_id = 1U;
    denoise.kind = Flux2KleinPhysicalPhaseKind::iterative_denoise;
    denoise.dependencies = {0U};
    denoise.topology_fingerprint = topology_fingerprint;
    denoise.hardware_resource_id = hardware_resource_id;
    denoise.prepared_resources.push_back(
        PreparedResourceRequirement{
            std::string(flux2_klein_denoiser_resource_id),
            std::nullopt});
    denoise.iterative_work = IterativePhysicalInvocation{
        semantics.iteration_count,
        semantics.batch,
        topology_fingerprint,
        hardware_resource_id,
    };
    denoise.input_value_ids = {
        semantics.values.initial_latent.id,
        semantics.values.noise.id,
        semantics.values.positive_conditioning.id,
        semantics.values.negative_conditioning.id,
        semantics.values.sigma_schedule.id,
    };
    denoise.output_value_ids = {
        semantics.values.sampled_latent.id,
    };
    plan.phases.push_back(std::move(denoise));

    Flux2KleinPhysicalPhase decode;
    decode.phase_id = 2U;
    decode.kind = Flux2KleinPhysicalPhaseKind::decode_image;
    decode.dependencies = {1U};
    decode.topology_fingerprint = topology_fingerprint;
    decode.hardware_resource_id = hardware_resource_id;
    decode.prepared_resources.push_back(
        PreparedResourceRequirement{
            std::string(flux2_klein_vae_resource_id),
            std::nullopt});
    decode.input_value_ids = {
        semantics.values.sampled_latent.id,
    };
    decode.output_value_ids = {
        semantics.values.decoded_image.id,
    };
    plan.phases.push_back(std::move(decode));

    const auto status =
        validate_flux2_klein_component_plan(semantics, plan);
    if (!status) return status;
    return plan;
}

Status validate_flux2_klein_component_plan(
    const Flux2KleinSemanticContract& semantics,
    const Flux2KleinPhysicalPlan& plan) {
    const auto semantic_status =
        validate_flux2_klein_oracle_semantics(semantics);
    if (!semantic_status) return semantic_status;

    if (plan.phases.size() != 3U) {
        return Status::invalid_argument(
            "qualified FLUX.2 component plan requires exactly three component phases");
    }

    const std::array<Flux2KleinPhysicalPhaseKind, 3> expected_kinds{
        Flux2KleinPhysicalPhaseKind::conditioning,
        Flux2KleinPhysicalPhaseKind::iterative_denoise,
        Flux2KleinPhysicalPhaseKind::decode_image,
    };
    const std::array<std::string_view, 3> expected_resources{
        flux2_klein_text_encoder_resource_id,
        flux2_klein_denoiser_resource_id,
        flux2_klein_vae_resource_id,
    };

    std::string topology;
    std::string hardware_resource;
    for (std::size_t i = 0; i < plan.phases.size(); ++i) {
        const auto& phase = plan.phases[i];
        if (phase.phase_id != i || phase.kind != expected_kinds[i]) {
            return Status::invalid_argument(
                "FLUX.2 component phase ordering/identity differs from qualified plan");
        }
        if (phase.topology_fingerprint.empty() ||
            phase.hardware_resource_id.empty()) {
            return Status::invalid_argument(
                "FLUX.2 component phase placement must be explicit");
        }
        if (i == 0U) {
            topology = phase.topology_fingerprint;
            hardware_resource = phase.hardware_resource_id;
        } else if (
            phase.topology_fingerprint != topology ||
            phase.hardware_resource_id != hardware_resource) {
            return Status::invalid_argument(
                "Stage 8F qualified component plan requires one selected placement");
        }

        const auto resource_status =
            validate_prepared_resource_requirements(
                phase.prepared_resources);
        if (!resource_status) return resource_status;
        if (phase.prepared_resources.size() != 1U ||
            phase.prepared_resources.front().resource_id !=
                expected_resources[i]) {
            return Status::invalid_argument(
                "FLUX.2 component phase prepared-resource identity mismatch");
        }
        if (phase.prepared_resources.front().expected_device_bytes) {
            return Status::invalid_argument(
                "Stage 8F must not invent per-component resident-byte requirements");
        }

        const auto value_status =
            validate_value_ids(semantics, phase);
        if (!value_status) return value_status;
    }

    if (!plan.phases[0].dependencies.empty() ||
        plan.phases[1].dependencies != std::vector<std::uint32_t>{0U} ||
        plan.phases[2].dependencies != std::vector<std::uint32_t>{1U}) {
        return Status::invalid_argument(
            "FLUX.2 component phase dependencies differ from qualified ordering");
    }

    if (plan.phases[0].iterative_work ||
        !plan.phases[1].iterative_work ||
        plan.phases[2].iterative_work) {
        return Status::invalid_argument(
            "only the FLUX.2 denoiser phase may carry iterative work");
    }

    const auto& iterative = *plan.phases[1].iterative_work;
    const WorkloadPhysicalInvocation invocation{iterative};
    const auto invocation_status =
        validate_workload_physical_invocation_boundary(invocation);
    if (!invocation_status) return invocation_status;
    if (iterative.iteration_count != semantics.iteration_count ||
        iterative.active_instances != semantics.batch ||
        iterative.topology_fingerprint != topology ||
        iterative.hardware_resource_id != hardware_resource) {
        return Status::invalid_argument(
            "FLUX.2 denoiser iterative work does not match semantic/placement contract");
    }

    const auto& values = semantics.values;
    if (plan.phases[0].input_value_ids !=
            std::vector<std::string>{values.prompt_text.id} ||
        plan.phases[0].output_value_ids !=
            std::vector<std::string>{
                values.positive_conditioning.id,
                values.negative_conditioning.id} ||
        plan.phases[1].input_value_ids !=
            std::vector<std::string>{
                values.initial_latent.id,
                values.noise.id,
                values.positive_conditioning.id,
                values.negative_conditioning.id,
                values.sigma_schedule.id} ||
        plan.phases[1].output_value_ids !=
            std::vector<std::string>{values.sampled_latent.id} ||
        plan.phases[2].input_value_ids !=
            std::vector<std::string>{values.sampled_latent.id} ||
        plan.phases[2].output_value_ids !=
            std::vector<std::string>{values.decoded_image.id}) {
        return Status::invalid_argument(
            "FLUX.2 component semantic value handoff differs from qualified plan");
    }

    return Status::ok();
}

} // namespace air
