#include "air/flux2_klein_semantics.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace air {
namespace {

constexpr std::array<double, 5> kQualifiedSigmas{
    1.000000000,
    0.967383988,
    0.908143923,
    0.767199964,
    0.000000000,
};

constexpr std::array<Flux2KleinSemanticOperationKind, 7>
    kQualifiedOperationSequence{
        Flux2KleinSemanticOperationKind::encode_conditioning,
        Flux2KleinSemanticOperationKind::derive_negative_conditioning,
        Flux2KleinSemanticOperationKind::initialize_latent,
        Flux2KleinSemanticOperationKind::derive_schedule,
        Flux2KleinSemanticOperationKind::realize_seeded_noise,
        Flux2KleinSemanticOperationKind::iterative_sample,
        Flux2KleinSemanticOperationKind::decode_latent,
    };

bool valid_value_identity(
    const Flux2KleinSemanticValueIdentity& value,
    Flux2KleinSemanticValueKind expected_kind) {
    return value.kind == expected_kind && !value.id.empty();
}

} // namespace

const char* to_string(Flux2KleinSemanticValueKind kind) noexcept {
    switch (kind) {
    case Flux2KleinSemanticValueKind::prompt_text:
        return "prompt-text";
    case Flux2KleinSemanticValueKind::conditioning:
        return "conditioning";
    case Flux2KleinSemanticValueKind::seed:
        return "seed";
    case Flux2KleinSemanticValueKind::noise:
        return "noise";
    case Flux2KleinSemanticValueKind::sigma_schedule:
        return "sigma-schedule";
    case Flux2KleinSemanticValueKind::latent:
        return "latent";
    case Flux2KleinSemanticValueKind::image:
        return "image";
    }
    return "unknown";
}

const char* to_string(Flux2KleinSemanticOperationKind kind) noexcept {
    switch (kind) {
    case Flux2KleinSemanticOperationKind::encode_conditioning:
        return "encode-conditioning";
    case Flux2KleinSemanticOperationKind::derive_negative_conditioning:
        return "derive-negative-conditioning";
    case Flux2KleinSemanticOperationKind::initialize_latent:
        return "initialize-latent";
    case Flux2KleinSemanticOperationKind::derive_schedule:
        return "derive-schedule";
    case Flux2KleinSemanticOperationKind::realize_seeded_noise:
        return "realize-seeded-noise";
    case Flux2KleinSemanticOperationKind::iterative_sample:
        return "iterative-sample";
    case Flux2KleinSemanticOperationKind::decode_latent:
        return "decode-latent";
    }
    return "unknown";
}

const char* to_string(Flux2KleinSamplerKind kind) noexcept {
    switch (kind) {
    case Flux2KleinSamplerKind::euler:
        return "euler";
    }
    return "unknown";
}

Flux2KleinSemanticContract qualified_flux2_klein_oracle_semantics() {
    Flux2KleinSemanticContract contract;
    contract.width = 1024U;
    contract.height = 1024U;
    contract.batch = 1U;
    contract.iteration_count = 4U;
    contract.seed = 432262096973490ULL;
    contract.sampler = Flux2KleinSamplerKind::euler;
    contract.cfg = 1.0;

    contract.latent_geometry = Flux2KleinLatentGeometry{
        1U, 128U, 64U, 64U};
    contract.sigma_schedule = kQualifiedSigmas;

    contract.values.prompt_text = {
        Flux2KleinSemanticValueKind::prompt_text,
        "flux2-klein/prompt-text"};
    contract.values.positive_conditioning = {
        Flux2KleinSemanticValueKind::conditioning,
        "flux2-klein/conditioning/positive"};
    contract.values.negative_conditioning = {
        Flux2KleinSemanticValueKind::conditioning,
        "flux2-klein/conditioning/negative"};
    contract.values.seed = {
        Flux2KleinSemanticValueKind::seed,
        "flux2-klein/seed"};
    contract.values.noise = {
        Flux2KleinSemanticValueKind::noise,
        "flux2-klein/noise"};
    contract.values.sigma_schedule = {
        Flux2KleinSemanticValueKind::sigma_schedule,
        "flux2-klein/sigma-schedule"};
    contract.values.initial_latent = {
        Flux2KleinSemanticValueKind::latent,
        "flux2-klein/latent/initial"};
    contract.values.sampled_latent = {
        Flux2KleinSemanticValueKind::latent,
        "flux2-klein/latent/sampled"};
    contract.values.decoded_image = {
        Flux2KleinSemanticValueKind::image,
        "flux2-klein/image/decoded"};

    contract.operation_sequence = kQualifiedOperationSequence;
    return contract;
}

Status validate_flux2_klein_oracle_semantics(
    const Flux2KleinSemanticContract& contract) {
    if (contract.width != 1024U ||
        contract.height != 1024U ||
        contract.batch != 1U ||
        contract.iteration_count != 4U) {
        return Status::invalid_argument(
            "FLUX.2 Klein semantic contract does not match qualified oracle geometry/work");
    }
    if (contract.seed != 432262096973490ULL) {
        return Status::invalid_argument(
            "FLUX.2 Klein semantic contract seed differs from qualified oracle");
    }
    if (contract.sampler != Flux2KleinSamplerKind::euler ||
        !std::isfinite(contract.cfg) ||
        contract.cfg != 1.0) {
        return Status::invalid_argument(
            "FLUX.2 Klein semantic sampler/CFG differs from qualified oracle");
    }
    if (contract.latent_geometry.batch != 1U ||
        contract.latent_geometry.channels != 128U ||
        contract.latent_geometry.height != 64U ||
        contract.latent_geometry.width != 64U) {
        return Status::invalid_argument(
            "FLUX.2 Klein latent semantic geometry differs from qualified oracle");
    }
    if (contract.sigma_schedule != kQualifiedSigmas) {
        return Status::invalid_argument(
            "FLUX.2 Klein sigma schedule differs from qualified oracle");
    }
    if (contract.operation_sequence != kQualifiedOperationSequence) {
        return Status::invalid_argument(
            "FLUX.2 Klein semantic operation sequence differs from qualified oracle");
    }

    const std::array<const Flux2KleinSemanticValueIdentity*, 9> values{
        &contract.values.prompt_text,
        &contract.values.positive_conditioning,
        &contract.values.negative_conditioning,
        &contract.values.seed,
        &contract.values.noise,
        &contract.values.sigma_schedule,
        &contract.values.initial_latent,
        &contract.values.sampled_latent,
        &contract.values.decoded_image,
    };
    const std::array<Flux2KleinSemanticValueKind, 9> expected_kinds{
        Flux2KleinSemanticValueKind::prompt_text,
        Flux2KleinSemanticValueKind::conditioning,
        Flux2KleinSemanticValueKind::conditioning,
        Flux2KleinSemanticValueKind::seed,
        Flux2KleinSemanticValueKind::noise,
        Flux2KleinSemanticValueKind::sigma_schedule,
        Flux2KleinSemanticValueKind::latent,
        Flux2KleinSemanticValueKind::latent,
        Flux2KleinSemanticValueKind::image,
    };

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!valid_value_identity(*values[i], expected_kinds[i])) {
            return Status::invalid_argument(
                "FLUX.2 Klein semantic value identity is missing or has wrong kind");
        }
        for (std::size_t j = i + 1U; j < values.size(); ++j) {
            if (values[i]->id == values[j]->id) {
                return Status::invalid_argument(
                    "FLUX.2 Klein semantic value identities must be unique");
            }
        }
    }

    return Status::ok();
}

} // namespace air
