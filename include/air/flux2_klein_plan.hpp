#pragma once

#include "air/flux2_klein_semantics.hpp"
#include "air/invocation.hpp"
#include "air/resource.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace air {

// Exact artifact-qualified prepared-resource identities from Prompt 7A/7B.
// Generic AIR resource logic treats these as opaque equality keys.
inline constexpr std::string_view flux2_klein_text_encoder_resource_id =
    "model/flux2-klein/text-encoder/sha256/"
    "3eab03a77adb0ee5304a4e677d5c10ac22f9049c1d7c894adca4f8bb39206ca8";

inline constexpr std::string_view flux2_klein_denoiser_resource_id =
    "model/flux2-klein/denoiser/sha256/"
    "97ed34fe0567e436200f2faee3939b88f2b5d99f8af2a4dc16532c4245c0ccb6";

inline constexpr std::string_view flux2_klein_vae_resource_id =
    "model/flux2-klein/vae/sha256/"
    "868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3";

enum class Flux2KleinPhysicalPhaseKind {
    conditioning = 0,
    iterative_denoise,
    decode_image,
};

[[nodiscard]] const char* to_string(
    Flux2KleinPhysicalPhaseKind kind) noexcept;

// Adapter-specific component phase. It references semantic value identities but
// does not interpret their storage. Unknown work is left unknown rather than
// converted into a fake universal work count.
struct Flux2KleinPhysicalPhase {
    std::uint32_t phase_id{0};
    Flux2KleinPhysicalPhaseKind kind{
        Flux2KleinPhysicalPhaseKind::conditioning};
    std::vector<std::uint32_t> dependencies;

    std::string topology_fingerprint;
    std::string hardware_resource_id;

    std::vector<PreparedResourceRequirement> prepared_resources;

    // Present only for the qualified iterative denoising region in this slice.
    std::optional<IterativePhysicalInvocation> iterative_work;

    std::vector<std::string> input_value_ids;
    std::vector<std::string> output_value_ids;
};

struct Flux2KleinPhysicalPlan {
    std::vector<Flux2KleinPhysicalPhase> phases;
};

// This is a deterministic projection after placement has already been selected
// by AIR's existing authorities. It is not a planner and makes no residency,
// transition-time, stream, overlap, or eviction decision.
[[nodiscard]] Result<Flux2KleinPhysicalPlan>
make_flux2_klein_component_plan(
    const Flux2KleinSemanticContract& semantics,
    std::string topology_fingerprint,
    std::string hardware_resource_id);

[[nodiscard]] Status validate_flux2_klein_component_plan(
    const Flux2KleinSemanticContract& semantics,
    const Flux2KleinPhysicalPlan& plan);

} // namespace air
