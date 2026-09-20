#pragma once

#include "DecodedGeometry.h"

#include <cstddef>
#include <span>

namespace CesiumI3SContent {

/**
 * @brief Decodes Draco-compressed I3S node geometry into DecodedGeometry.
 *
 * I3S Draco buffers begin with the magic bytes "DRACO" and are decoded using
 * the draco::Decoder. Vertex positions are in the same geographic-offset space
 * as raw binary geometry. Scale factors (scaleX/scaleY) are read from Draco
 * attribute metadata when present.
 */
struct I3SDracoDecoder {
  /**
   * @brief Decode a Draco-compressed I3S geometry buffer.
   *
   * If the buffer does not start with the Draco magic bytes the result will
   * contain an error.
   *
   * @param data Full Draco buffer bytes (starts with "DRACO").
   */
  static DecodedGeometry decode(std::span<const std::byte> data);
};

} // namespace CesiumI3SContent
