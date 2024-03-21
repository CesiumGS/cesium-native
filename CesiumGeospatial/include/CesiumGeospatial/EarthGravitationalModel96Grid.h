#pragma once

#include "Library.h"

#include <CesiumGeospatial/Cartographic.h>

#include <GSL/span>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGeospatial {

/**
 * @brief Allows elevation data in the format provided by the Earth
 * Gravitational Model 1996 to be loaded and queried.
 */
class CESIUMGEOSPATIAL_API EarthGravitationalModel96Grid final {
public:
  /**
   * Attempts to create a {@link EarthGravitationalModel96Grid} from the given file.
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
   * Samples the height at the given position.
   *
   * @param position The position to sample.
   * @returns The height representing the difference in meters of mean sea-level from the
   * surface of a WGS84 ellipsoid.
   */
  double sampleHeight(Cartographic& position) const;

private:
  EarthGravitationalModel96Grid(const std::vector<int16_t>& gridValues);

  /**
   * Returns the height of the given grid value, in meters.
   */
  double getHeightForIndices(const int vertical, const int horizontal) const;

  /**
   * Ensures latitude is in the range(-PI, PI) with an exclusive upper bound -
   * PI will always map to -PI
   */
  double mapLatitude(double latitude) const;

  /**
   * Ensures longitude is in the range (0, 2PI) with an exclusive lower bound -
   * 0 will always map to 2PI
   */
  double mapLongitude(double longitude) const;

  std::vector<int16_t> _gridValues;
};

} // namespace CesiumGeospatial
