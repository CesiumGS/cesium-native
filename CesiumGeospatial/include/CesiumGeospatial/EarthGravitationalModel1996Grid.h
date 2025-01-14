#pragma once

#include <CesiumGeospatial/Library.h>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace CesiumGeospatial {

class Cartographic;

/**
 * @brief Loads and queries heights from an Earth Gravitational Model 1996
 * (EGM96) grid.
 *
 * EGM96 is a standard geopotential model of the earth's surface, which can be
 * used to obtain an approximation of the mean sea level (MSL) height at any
 * location on Earth.
 */
class CESIUMGEOSPATIAL_API EarthGravitationalModel1996Grid final {
public:
  /**
   * @brief Attempts to create a {@link EarthGravitationalModel1996Grid} from the given buffer.
   *
   * This method expects the buffer to contain the contents of the WW15MGH.DAC
   * 15-arcminute grid. It must be at least 721 * 1440 * 2 = 2,076,480 bytes.
   * Any additional bytes at the end of the buffer are ignored.
   *
   * @returns The instance created from the buffer, or `std::nullopt` if the
   * buffer cannot be interpreted as an EGM96 grid.
   */
  static std::optional<EarthGravitationalModel1996Grid>
  fromBuffer(const std::span<const std::byte>& buffer);

  /**
   * @brief Samples the height at the given position.
   *
   * @param position The position to sample. The `height` is ignored.
   * @returns The height (in meters) of the EGM96 surface above the WGS84
   * ellipsoid. A positive value indicates that EGM96 is above the ellipsoid's
   * surface, while a negative value indicates that EGM96 is below the
   * ellipsoid's surface.
   */
  double sampleHeight(const Cartographic& position) const;

private:
  EarthGravitationalModel1996Grid(std::vector<int16_t>&& gridValues);

  /**
   * Returns the height of the given grid value, in meters.
   */
  double getHeightForIndices(size_t vertical, size_t horizontal) const;

  std::vector<int16_t> _gridValues;
};

} // namespace CesiumGeospatial
