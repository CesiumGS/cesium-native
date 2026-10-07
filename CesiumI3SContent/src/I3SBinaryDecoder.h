#pragma once

#include "DecodedGeometry.h"

#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/GeometrySchema.h>

#include <cstddef>
#include <span>

namespace CesiumI3SContent {

/**
 * @brief Decodes raw (non-Draco) I3S node geometry binaries into
 * DecodedGeometry.
 *
 * Two paths are supported:
 * - **Legacy** (I3S < 1.7): header + attributes ordered per
 *   GeometrySchema.
 * - **Modern** (I3S >= 1.7): standard 8-byte header + attributes ordered per
 *   GeometryBuffer descriptor. Callers must check for Draco/LEPCC before
 *   calling decodeModern() — this function only handles raw binary.
 */
struct I3SBinaryDecoder {
  /**
   * @brief Decode a legacy binary geometry buffer.
   * @param data Full buffer bytes.
   * @param schema The GeometrySchema from the layer's store.
   */
  static DecodedGeometry decodeLegacy(
      std::span<const std::byte> data,
      const CesiumI3S::GeometrySchema& schema);

  /**
   * @brief Decode a modern (1.7+) raw binary geometry buffer.
   *
   * Returns an error result if the buffer begins with the Draco magic bytes
   * or if the GeometryBuffer descriptor specifies LEPCC encoding.
   *
   * @param data Full buffer bytes.
   * @param geometryBuffer The GeometryBuffer descriptor from the layer's
   *   GeometryDefinition.
   */
  static DecodedGeometry decodeModern(
      std::span<const std::byte> data,
      const CesiumI3S::GeometryBuffer& geometryBuffer);
};

} // namespace CesiumI3SContent
