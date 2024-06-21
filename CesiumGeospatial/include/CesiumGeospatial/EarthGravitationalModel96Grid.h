#pragma once

#include "Library.h"

#include <CesiumGeospatial/Cartographic.h>

#include <gsl/span>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGeospatial {

/**
 * @brief Loads and queries heights from an Earth Gravitational Model 96 (EGM96) grid.
 * 
 * EGM96 is a standard geopotential model of the earth's surface, which can be used to obtain an approximation of the mean sea level (MSL) at any location on a WGS84 ellipsoid.
 */
class CESIUMGEOSPATIAL_API EarthGravitationalModel96Grid final {
public:
  /**
   * @brief Attempts to create a {@link EarthGravitationalModel96Grid} from the given file.
   *
   * This method expects a file in the format of the WW15MGH.DAC 15-arcminute
   * grid.
   */
  static std::optional<EarthGravitationalModel96Grid>
  fromFile(const std::string& filename);

  /**
   * Attempts to create a {@link EarthGravitationalModel96Grid} from the given buffer.
   *
   * This method expects the buffer to contain the contents of the WW15MGH.DAC
   * 15-arcminute grid.
   */
  static std::optional<EarthGravitationalModel96Grid>
  fromBuffer(const gsl::span<const std::byte>& buffer);

  /**
   * @brief Samples the height at the given position.
   *
   * @param position The position to sample.
   * @returns The height (in meters) of the EGM96 surface above the WGS84 ellipsoid. 
   *          A positive value indicates that MSL is above the ellipsoid's surface,
   *          while a negative value indicates that MSL is below the ellipsoid's surface.
   */
  double sampleHeight(const Cartographic& position) const;

private:
  EarthGravitationalModel96Grid(std::vector<int16_t>&& gridValues);

  /**
   * Returns the height of the given grid value, in meters.
   */
  double getHeightForIndices(const int vertical, const int horizontal) const;

  std::vector<int16_t> _gridValues;
};

} // namespace CesiumGeospatial
