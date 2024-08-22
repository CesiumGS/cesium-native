#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

#include <algorithm>
#include <fstream>
#include <vector>

using namespace CesiumUtility;

namespace CesiumGeospatial {

namespace {

// WW15MGH.DAC has 721 rows representing the range (90N, 90S) and 1440 columns
// representing the range (0E, 360E)

// The number of rows in the file
const size_t NUM_ROWS = 721;
// The number of columns in the file
const size_t NUM_COLUMNS = 1440;

} // namespace

std::optional<EarthGravitationalModel1996Grid>
CesiumGeospatial::EarthGravitationalModel1996Grid::fromFile(
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

std::optional<EarthGravitationalModel1996Grid>
CesiumGeospatial::EarthGravitationalModel1996Grid::fromBuffer(
    const gsl::span<const std::byte>& buffer) {
  const size_t expectedValues = NUM_ROWS * NUM_COLUMNS;
  const size_t expectedBytes = expectedValues * sizeof(int16_t);

  if (buffer.size_bytes() < expectedBytes) {
    // Not enough data - is this a valid WW15MGH.DAC?
    return std::nullopt;
  }

  std::vector<int16_t> gridValues;
  gridValues.resize(expectedValues);

  const std::byte* pRead = buffer.data();
  std::byte* pWrite = reinterpret_cast<std::byte*>(gridValues.data());

  for (size_t i = 0; i < expectedBytes; i += 2) {
    // WW15MGH.DAC is in big endian, so we swap the bytes
    pWrite[i] = pRead[i + 1];
    pWrite[i + 1] = pRead[i];
  }

  return EarthGravitationalModel1996Grid(std::move(gridValues));
}

double EarthGravitationalModel1996Grid::sampleHeight(
    const Cartographic& position) const {
  const double longitude = Math::zeroToTwoPi(position.longitude);
  const double latitude =
      Math::clamp(position.latitude, -Math::PiOverTwo, Math::PiOverTwo);

  const double horizontalIndexDecimal = (NUM_COLUMNS * longitude) / Math::TwoPi;
  const size_t horizontalIndex = static_cast<size_t>(horizontalIndexDecimal);

  const double verticalIndexDecimal =
      ((NUM_ROWS - 1) * (Math::PiOverTwo - latitude)) / Math::OnePi;
  const size_t verticalIndex = static_cast<size_t>(verticalIndexDecimal);

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

EarthGravitationalModel1996Grid::EarthGravitationalModel1996Grid(
    std::vector<int16_t>&& gridValues)
    : _gridValues(gridValues) {}

double EarthGravitationalModel1996Grid::getHeightForIndices(
    const size_t horizontal,
    const size_t vertical) const {

  size_t clampedVertical = vertical;
  if (vertical >= NUM_ROWS) {
    clampedVertical = NUM_ROWS - 1;
  }

  const size_t index = clampedVertical * NUM_COLUMNS + horizontal;
  const double result = _gridValues[index] / 100.0;

  return result;
}

} // namespace CesiumGeospatial
