#pragma once
#include <CesiumI3S/AttributeStorage.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/Library.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Feature data record (featureData.cmn.md). */
struct CESIUMI3S_API FeatureData {
  /** @brief Feature ID. */
  uint64_t id = 0;
  /** @brief Feature position as [x, y, z]. */
  std::optional<std::vector<double>> position;
  /** @brief Offset from the feature's pivot point. */
  std::optional<std::array<double, 3>> pivotOffset;
  /** @brief Minimum bounding box [xmin, ymin, zmin, xmax, ymax, zmax]. */
  std::optional<std::array<double, 6>> mbb;
  /** @brief Name of the sublayer this feature belongs to. */
  std::optional<std::string> layer;
  /** @brief Per-feature attribute values. */
  std::optional<FeatureAttribute> attributes;
  /** @brief Geometries associated with this feature. */
  std::optional<std::vector<Geometry>> geometries;
};

} // namespace CesiumI3S
