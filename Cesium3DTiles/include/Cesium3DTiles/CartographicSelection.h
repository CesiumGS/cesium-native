#pragma once

#include "Cesium3DTiles/Library.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <glm/vec2.hpp>
#include <optional>
#include <vector>
#include <string>

namespace Cesium3DTiles {

/**
 * @brief A 2D polygon that can be rasterized onto {@link Tileset} objects.
 */
class CESIUM3DTILES_API CartographicSelection final {
public:
  CartographicSelection(
      const std::string& targetTextureName,
      const std::vector<glm::dvec2>& polygon,
      bool isForCulling);

  constexpr const std::string& getTargetTextureName() const {
    return this->_targetTextureName;
  }

  constexpr const std::vector<glm::dvec2>& getVertices() const {
    return this->_vertices;
  }

  constexpr const std::vector<uint32_t>& getIndices() const {
    return this->_indices;
  }

  constexpr const std::optional<CesiumGeospatial::GlobeRectangle>&
  getBoundingRectangle() const {
    return this->_boundingRectangle;
  }

  constexpr bool isForCulling() const { return this->_isForCulling; }

private:
  std::string _targetTextureName;
  std::vector<glm::dvec2> _vertices;
  std::vector<uint32_t> _indices;
  std::optional<CesiumGeospatial::GlobeRectangle> _boundingRectangle;
  bool _isForCulling;
};

} // namespace Cesium3DTiles
