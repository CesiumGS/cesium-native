#pragma once

#include <CesiumGltf/ExtensionExtMeshPrimitiveEdgeVisibilitySpec.h>

namespace CesiumGltf {

/** @copydoc ExtensionExtMeshPrimitiveEdgeVisibilitySpec */
struct CESIUMGLTF_API ExtensionExtMeshPrimitiveEdgeVisibility final
    : public ExtensionExtMeshPrimitiveEdgeVisibilitySpec {
  /**
   * @brief Known values for the visibility specified for a single edge.
   */
  struct Visibility {
    /** @brief Hidden edge - the edge should never be drawn */
    static constexpr uint8_t HIDDEN = 0;

    /** @brief Silhouette edge - the edge should be drawn only in silhouette
     * (i.e., when separating a front-facing triangle from a back-facing
     * triangle) */
    static constexpr uint8_t SILHOUETTE = 1;

    /** @brief Hard edge - the edge should always be drawn. */
    static constexpr uint8_t HARD_EDGE = 2;

    /** @brief Repeated hard edge - the edge should always be drawn, and its
     * visibility is already encoded as HARD_EDGE for an adjoining triangle. */
    static constexpr uint8_t REPEATED_HARD_EDGE = 3;
  };

  ExtensionExtMeshPrimitiveEdgeVisibility() = default;
};

} // namespace CesiumGltf
