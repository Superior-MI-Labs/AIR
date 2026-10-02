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


Result<Flux2KleinLatentGeometry>
derive_flux2_klein_latent_geometry(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t batch) {
    if (width == 0U || height == 0U || batch == 0U) {
        return Status::invalid_argument(
            "FLUX.2 Klein latent geometry requires positive width/height/batch");
    }
    if ((width % 16U) != 0U || (height % 16U) != 0U) {
        return Status::invalid_argument(
            "FLUX.2 Klein qualified latent geometry requires dimensions divisible by 16");
    }
    return Flux2KleinLatentGeometry{
        batch,
        128U,
        height / 16U,
        width / 16U,
    };
}

Result<Flux2KleinScheduleDerivation>
derive_flux2_klein_schedule(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t steps) {
    if (width == 0U || height == 0U || steps == 0U) {
        return Status::invalid_argument(
            "FLUX.2 Klein schedule requires positive width/height/steps");
    }

    const long double pixels =
        static_cast<long double>(width) *
        static_cast<long double>(height);
    const long double sequence =
        pixels / static_cast<long double>(16U * 16U);
    if (!std::isfinite(static_cast<double>(sequence))) {
        return Status::invalid_argument(
            "FLUX.2 Klein schedule image sequence length is not finite");
    }

    const auto image_sequence_length =
        static_cast<std::uint64_t>(std::llround(sequence));

    constexpr double a1 = 8.73809524e-05;
    constexpr double b1 = 1.89833333;
    constexpr double a2 = 0.00016927;
    constexpr double b2 = 0.45666666;

    const double image_seq =
        static_cast<double>(image_sequence_length);

    double mu = 0.0;
    if (image_sequence_length > 4300U) {
        mu = a2 * image_seq + b2;
    } else {
        const double m200 = a2 * image_seq + b2;
        const double m10 = a1 * image_seq + b1;
        const double a = (m200 - m10) / 190.0;
        const double b = m200 - 200.0 * a;
        mu = a * static_cast<double>(steps) + b;
    }

    const double e_mu = std::exp(mu);
    if (!std::isfinite(mu) || !std::isfinite(e_mu) || e_mu <= 0.0) {
        return Status::data_error(
            "FLUX.2 Klein schedule derivation produced non-finite shift");
    }

    Flux2KleinScheduleDerivation result;
    result.image_sequence_length = image_sequence_length;
    result.mu = mu;
    result.sigmas.reserve(static_cast<std::size_t>(steps) + 1U);

    for (std::uint32_t i = 0U; i <= steps; ++i) {
        const double t =
            1.0 - static_cast<double>(i) / static_cast<double>(steps);
        if (i == steps) {
            result.sigmas.push_back(0.0);
            continue;
        }
        const double sigma =
            e_mu / (e_mu + (1.0 / t - 1.0));
        if (!std::isfinite(sigma)) {
            return Status::data_error(
                "FLUX.2 Klein schedule derivation produced non-finite sigma");
        }
        result.sigmas.push_back(sigma);
    }

    return result;
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

    const auto derived_geometry =
        derive_flux2_klein_latent_geometry(
            contract.width, contract.height, contract.batch);
    if (!derived_geometry ||
        derived_geometry.value().batch != contract.latent_geometry.batch ||
        derived_geometry.value().channels != contract.latent_geometry.channels ||
        derived_geometry.value().height != contract.latent_geometry.height ||
        derived_geometry.value().width != contract.latent_geometry.width) {
        return Status::data_error(
            "AIR FLUX.2 Klein latent derivation disagrees with qualified oracle");
    }

    if (contract.sigma_schedule != kQualifiedSigmas) {
        return Status::invalid_argument(
            "FLUX.2 Klein sigma schedule differs from qualified oracle");
    }

    const auto derived_schedule =
        derive_flux2_klein_schedule(
            contract.width, contract.height, contract.iteration_count);
    if (!derived_schedule ||
        derived_schedule.value().image_sequence_length != 4096U ||
        derived_schedule.value().sigmas.size() !=
            contract.sigma_schedule.size()) {
        return Status::data_error(
            "AIR FLUX.2 Klein schedule derivation disagrees with qualified oracle structure");
    }
    for (std::size_t i = 0; i < contract.sigma_schedule.size(); ++i) {
        if (std::abs(
                derived_schedule.value().sigmas[i] -
                contract.sigma_schedule[i]) > 1.0e-9) {
            return Status::data_error(
                "AIR FLUX.2 Klein schedule derivation disagrees with qualified oracle values");
        }
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
