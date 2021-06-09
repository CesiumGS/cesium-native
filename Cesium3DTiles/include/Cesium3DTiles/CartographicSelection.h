#pragma once

#include "Cesium3DTiles/Library.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTiles {

/**
 * @brief A 2D polygon that can be rasterized onto {@link Tileset} objects.
 */
class CESIUM3DTILES_API CartographicSelection final {
public:
  /**
   * @brief Constructs a 2D polygon that can be rasterized onto {@link Tileset}
   * objects.
   *
   * @param targetTextureName The desired name of the target texture to be
   * rasterized. The tile glTFs containing this polygon will carry a texture id
   * in their model.extras by the name of CUSTOM_MASK_<targetTextureName>.
   * @param polygon An array of longitude-latitude points in WGS84 defining the
   * perimeter of the 2D polygon.
   * @param isForCulling Whether this selection is going to be used strictly
   * for culling. If it will only be used for culling inside the polygon, the
   * streaming can be optimized by avoiding loading tiles that are entirely
   * within the culled area.
   */
  CartographicSelection(
      const std::string& targetTextureName,
      const std::vector<glm::dvec2>& polygon,
      bool isForCulling);

  /**
   * @brief Returns the target texture name that the polygon will be rasterized
   * into. The tile glTFs containing this polygon will carry a texture id in
   * their model.extras by the name of CUSTOM_MASK_<targetTextureName>.
   *
   * @return The target texture name.
   */
  constexpr const std::string& getTargetTextureName() const {
    return this->_targetTextureName;
  }

  /**
   * @brief Returns the longitude-latitude vertices in WGS84 that define the
   * perimeter of the selected polygon.
   *
   * @return The perimeter vertices in WGS84 longitude-latitude.
   */
  constexpr const std::vector<glm::dvec2>& getVertices() const {
    return this->_vertices;
  }

  /**
   * @brief Returns the triangulated indices representing a triangle
   * decomposition of the polygon. The indices are in reference to the
   * polygon's perimeter vertices.
   *
   * @return The indices for the polygon's triangle decomposition.
   */
  constexpr const std::vector<uint32_t>& getIndices() const {
    return this->_indices;
  }

  /**
   * @brief Returns a {@link GlobeRectangle} that represents the bounding
   * rectangle of the polygon in WGS84 longitude-latitude.
   *
   * @return The polygon's global bounding rectangle.
   */
  constexpr const std::optional<CesiumGeospatial::GlobeRectangle>&
  getBoundingRectangle() const {
    return this->_boundingRectangle;
  }

  /**
   * @brief Returns whether this selection is going to be used strictly for
   * culling. If it will only be used for culling inside the polygon, the
   * streaming can be optimized by avoiding loading tiles that are entirely
   * within the culled area.
   *
   * @return Whether this selection will only be used for culling.
   */
  constexpr bool isForCulling() const { return this->_isForCulling; }

private:
  std::string _targetTextureName;
  std::vector<glm::dvec2> _vertices;
  std::vector<uint32_t> _indices;
  std::optional<CesiumGeospatial::GlobeRectangle> _boundingRectangle;
  bool _isForCulling;
};

} // namespace Cesium3DTiles
