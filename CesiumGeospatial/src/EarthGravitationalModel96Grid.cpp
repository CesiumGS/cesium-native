#include <CesiumGeospatial/EarthGravitationalModel96Grid.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

#include <algorithm>
#include <fstream>
#include <vector>

using namespace CesiumUtility;

namespace CesiumGeospatial {

// WW15MGH.DAC has 721 rows representing the range (90N, 90S) and 1440 columns
// representing the range (0E, 360E)

// The number of rows in the file
const int NUM_ROWS = 721;
// The number of columns in the file
const int NUM_COLUMNS = 1440;

std::optional<EarthGravitationalModel96Grid>
CesiumGeospatial::EarthGravitationalModel96Grid::fromFile(
    const std::string& filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file.good()) {
    return std::nullopt;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), size);
  return fromBuffer(buffer);
}

std::optional<EarthGravitationalModel96Grid>
CesiumGeospatial::EarthGravitationalModel96Grid::fromBuffer(
    const gsl::span<const std::byte>& buffer) {
  const size_t expectedBytes = NUM_ROWS * NUM_COLUMNS * sizeof(int16_t);

  if (
      // Not enough data - is this a valid WW15MGH.DAC?
      buffer.size_bytes() < expectedBytes ||
      // WW15MGH.DAC should just be full of 2-byte int16_t, so if it's not even
      // then something has gone wrong (so we should stop now before we overflow
      // the end of the gridValues buffer!)
      buffer.size_bytes() % 2 != 0) {
    return std::nullopt;
  }

  const size_t size = buffer.size_bytes();
  const std::byte* pRead = buffer.data();
  std::vector<int16_t> gridValues;
  gridValues.resize(size / 2);
  std::byte* pWrite = reinterpret_cast<std::byte*>(gridValues.data());

  for (size_t i = 0; i < size; i += 2) {
    // WW15MGH.DAC is in big endian, so we swap the bytes
    pWrite[i] = pRead[i + 1];
    pWrite[i + 1] = pRead[i];
  }

  return EarthGravitationalModel96Grid(std::move(gridValues));
}

double EarthGravitationalModel96Grid::sampleHeight(
    const Cartographic& position) const {
  const double longitude = Math::zeroToTwoPi(position.longitude);
  const double latitude =
      Math::clamp(position.latitude, -Math::PiOverTwo, Math::PiOverTwo);

  const double horizontalIndexDecimal = (NUM_COLUMNS * longitude) / Math::TwoPi;
  const int horizontalIndex = static_cast<int>(horizontalIndexDecimal);

  const double verticalIndexDecimal =
      ((NUM_ROWS - 1) * (Math::PiOverTwo - latitude)) / Math::OnePi;
  const int verticalIndex = static_cast<int>(verticalIndexDecimal);

  // Get the normalized position of the coordinates within the grid tile
  const double xn = horizontalIndexDecimal - horizontalIndex;
  const double iyn = verticalIndexDecimal - verticalIndex;
  const double ixn = 1.0 - xn;
  const double yn = 1.0 - iyn;

  // Get surrounding grid points
  const double a1 = getHeightForIndices(horizontalIndex, verticalIndex);
  const double a2 = getHeightForIndices(horizontalIndex + 1, verticalIndex);
  const double b1 = getHeightForIndices(horizontalIndex, verticalIndex + 1);
  const double b2 = getHeightForIndices(horizontalIndex + 1, verticalIndex + 1);

  // Each component contributes to the total based on its normalized position in
  // the grid square
  const double result =
      (a1 * ixn * yn + a2 * xn * yn + b1 * ixn * iyn + b2 * xn * iyn);

  return result;
}

EarthGravitationalModel96Grid::EarthGravitationalModel96Grid(
    std::vector<int16_t>&& gridValues)
    : _gridValues(gridValues) {}

double EarthGravitationalModel96Grid::getHeightForIndices(
    const int horizontal,
    const int vertical) const {

  int clampedVertical = vertical;
  if (vertical > NUM_ROWS - 1) {
    clampedVertical = NUM_ROWS - 1;
  }

  const size_t index =
      static_cast<size_t>(clampedVertical * NUM_COLUMNS + horizontal);
  const double result = _gridValues[index] / 100.0;

  return result;
}

} // namespace CesiumGeospatial
