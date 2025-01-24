#pragma once

#include <cstdint>

namespace CesiumGltf {
/**
 * @brief Applies a sampler's WrapS value to the given U component of a texture
 * coordinate.
 *
 * @param u The U coordinate to apply the sampler wrap value to.
 * @param wrapS The sampler's WrapS value, matching a member of \ref
 * Sampler::WrapS, to apply.
 * @returns The U coordinate after applying the WrapS operation.
 */
double applySamplerWrapS(const double u, const int32_t wrapS);
/**
 * @brief Applies a sampler's WrapT value to the given V component of a texture
 * coordinate.
 *
 * @param v The V coordinate to apply the sampler wrap value to.
 * @param wrapT The sampler's WrapT value, matching a member of \ref
 * Sampler::WrapT, to apply.
 * @returns The V coordinate after applying the WrapT operation.
 */
double applySamplerWrapT(const double v, const int32_t wrapT);
} // namespace CesiumGltf
