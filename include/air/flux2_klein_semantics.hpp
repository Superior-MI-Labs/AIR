#pragma once

#include "air/result.hpp"
#include "air/status.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace air {

// These are adapter-owned semantic kinds for the one qualified FLUX.2 Klein
// oracle. They are deliberately not a universal AIR semantic-value catalog.
enum class Flux2KleinSemanticValueKind {
    prompt_text = 0,
    conditioning,
    seed,
    noise,
    sigma_schedule,
    latent,
    image,
};

enum class Flux2KleinSemanticOperationKind {
    encode_conditioning = 0,
    derive_negative_conditioning,
    initialize_latent,
    derive_schedule,
    realize_seeded_noise,
    iterative_sample,
    decode_latent,
};

enum class Flux2KleinSamplerKind {
    euler = 0,
};

[[nodiscard]] const char* to_string(
    Flux2KleinSemanticValueKind kind) noexcept;
[[nodiscard]] const char* to_string(
    Flux2KleinSemanticOperationKind kind) noexcept;
[[nodiscard]] const char* to_string(
    Flux2KleinSamplerKind kind) noexcept;

struct Flux2KleinSemanticValueIdentity {
    Flux2KleinSemanticValueKind kind{
        Flux2KleinSemanticValueKind::prompt_text};
    std::string id;
};

struct Flux2KleinLatentGeometry {
    std::uint32_t batch{0};
    std::uint32_t channels{0};
    std::uint32_t height{0};
    std::uint32_t width{0};
};

struct Flux2KleinScheduleDerivation {
    std::uint64_t image_sequence_length{0};
    double mu{0.0};
    std::vector<double> sigmas;
};

// AIR-owned implementations of the two deterministic semantic operations that
// can be qualified from retained Prompt 7 evidence without model execution.
// These are FLUX.2 Klein adapter functions, not a universal diffusion API.
[[nodiscard]] Result<Flux2KleinLatentGeometry>
derive_flux2_klein_latent_geometry(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t batch);

[[nodiscard]] Result<Flux2KleinScheduleDerivation>
derive_flux2_klein_schedule(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t steps);

struct Flux2KleinSemanticValues {
    Flux2KleinSemanticValueIdentity prompt_text;
    Flux2KleinSemanticValueIdentity positive_conditioning;
    Flux2KleinSemanticValueIdentity negative_conditioning;
    Flux2KleinSemanticValueIdentity seed;
    Flux2KleinSemanticValueIdentity noise;
    Flux2KleinSemanticValueIdentity sigma_schedule;
    Flux2KleinSemanticValueIdentity initial_latent;
    Flux2KleinSemanticValueIdentity sampled_latent;
    Flux2KleinSemanticValueIdentity decoded_image;
};

// Characterization contract for the frozen Prompt 7B/7C-H oracle.
//
// This represents known semantic meaning and semantically observable
// configuration only. It intentionally contains no storage dtype, tensor
// layout, device placement, stream, kernel, or DynamicVRAM policy.
struct Flux2KleinSemanticContract {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t batch{0};
    std::uint32_t iteration_count{0};
    std::uint64_t seed{0};
    Flux2KleinSamplerKind sampler{Flux2KleinSamplerKind::euler};
    double cfg{0.0};

    Flux2KleinLatentGeometry latent_geometry{};
    std::array<double, 5> sigma_schedule{};

    Flux2KleinSemanticValues values{};
    std::array<Flux2KleinSemanticOperationKind, 7> operation_sequence{};
};

[[nodiscard]] Flux2KleinSemanticContract
qualified_flux2_klein_oracle_semantics();

[[nodiscard]] Status validate_flux2_klein_oracle_semantics(
    const Flux2KleinSemanticContract& contract);

} // namespace air
