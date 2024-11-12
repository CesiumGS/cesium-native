#pragma once

#include <cstdint>

namespace CesiumGltf {
double applySamplerWrapS(const double u, const int32_t wrapS) noexcept;
double applySamplerWrapT(const double v, const int32_t wrapT) noexcept;
} // namespace CesiumGltf
