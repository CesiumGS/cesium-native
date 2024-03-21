#include <CesiumGeospatial/EarthGravitationalModel96Grid.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

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
// The total number of entries in the file
const int TOTAL_ENTRIES = NUM_ROWS * NUM_COLUMNS;

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
  const int expectedBytes = NUM_ROWS * NUM_COLUMNS * sizeof(int16_t);

  // Not enough data - is this a valid WW15MGH.DAC?
  if (buffer.size_bytes() < expectedBytes) {
    return std::nullopt;
  }

  std::vector<int16_t> gridValues;
  const int size = static_cast<int>(buffer.size_bytes());

  for (int i = 0; i < size; i += 2) {
    // WW15MGH.DAC is in big endian, so we swap the bytes
    const std::byte msb = buffer.data()[i];
    const std::byte lsb = buffer.data()[i + 1];
    const int16_t gridValue =
        static_cast<int16_t>(lsb) | static_cast<int16_t>(msb) << 8;
    gridValues.push_back(gridValue);
  }

  return EarthGravitationalModel96Grid(gridValues);
}

double
EarthGravitationalModel96Grid::sampleHeight(Cartographic& position) const {
  const double longitude = this->mapLongitude(position.longitude);
  const double latitude = this->mapLatitude(position.latitude);

  // const double quantizedLongitude =
  // floor(longitude / LONGITUDE_INTERVAL) * LONGITUDE_INTERVAL;

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
    const std::vector<int16_t>& gridValues)
    : _gridValues(gridValues) {}

double EarthGravitationalModel96Grid::getHeightForIndices(
    const int horizontal,
    const int vertical) const {

  int clampedVertical = vertical;
  if (vertical > NUM_ROWS - 1) {
    clampedVertical = NUM_ROWS - 1;
  }

  const std::vector<uint16_t>::size_type index = static_cast<std::vector<uint16_t>::size_type>(clampedVertical * NUM_COLUMNS + horizontal);
  const double result = static_cast<double>(_gridValues[index]) / 100.0;

  return result;
}

double EarthGravitationalModel96Grid::mapLatitude(double latitude) const {
  return latitude - Math::TwoPi * floor((latitude + Math::OnePi) / Math::TwoPi);
}

double EarthGravitationalModel96Grid::mapLongitude(double longitude) const {
  const double modTwoPi =
      fmod(fmod(longitude, Math::TwoPi) + Math::TwoPi, Math::TwoPi);

  if (Math::equalsEpsilon(modTwoPi, 0, Math::Epsilon14) &&
      !Math::equalsEpsilon(longitude, 0, Math::Epsilon14)) {
    return Math::TwoPi;
  }

  return modTwoPi;
}

} // namespace CesiumGeospatial
