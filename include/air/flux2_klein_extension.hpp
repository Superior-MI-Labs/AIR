#pragma once

#include "air/semantic_extension.hpp"

namespace air {

[[nodiscard]] SemanticPackageDeclaration
flux2_klein_semantic_package_declaration();

[[nodiscard]] Status register_flux2_klein_builtin_semantics(
    SemanticImplementationRegistry& registry);

} // namespace air
