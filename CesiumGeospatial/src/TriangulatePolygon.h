#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <vector>

namespace CesiumGeospatial {
/**
 * @brief Triangulates a polygon, returning the indices of the generated
 * triangles.
 *
 * @param rings The linear rings that make up this polygon. The first ring
 * defines the main polygon while the following rings define holes.
 */
std::vector<uint32_t>
triangulatePolygon(const std::vector<std::vector<glm::dvec2>>& rings);
} // namespace CesiumGeospatial