#include "I3SBinaryDecoder.h"
#include "I3SCoordinateTransform.h"
#include "I3SDracoDecoder.h"
#include "I3SGltfAssembler.h"

#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3SContent/I3SToGltfConverter.h>
#include <CesiumUtility/ErrorList.h>

#include <string>

namespace CesiumI3SContent {

namespace {

/**
 * @brief Validate the horizontal CRS. Only WGS84 geographic (WKID 4326) is
 * supported.
 */
bool validateHorizontalCrs(
    const CesiumI3S::SpatialReference& sr,
    CesiumUtility::ErrorList& errors) {
  const int32_t wkid = sr.horizontalWkid();
  // -1 means unspecified; assume WGS84 and proceed.
  if (wkid != -1 && wkid != 4326) {
    errors.emplaceError(
        "Unsupported horizontal CRS: WKID " + std::to_string(wkid) +
        ". Only WGS84 geographic (WKID 4326) is supported.");
    return false;
  }
  return true;
}

/**
 * @brief Validate and extract the height model.
 * Ellipsoidal, GravityRelatedHeight, and Orthometric are supported.
 * Returns the effective HeightModel enum value, or nullopt on error.
 */
std::optional<CesiumI3S::HeightModel> resolveHeightModel(
    const std::optional<CesiumI3S::HeightModelInfo>& hmi,
    CesiumUtility::ErrorList& errors) {
  if (!hmi.has_value() || !hmi->heightModel.has_value()) {
    // Default: treat as ellipsoidal (no geoid correction).
    return CesiumI3S::HeightModel::Ellipsoidal;
  }

  const CesiumI3S::HeightModel hm = *hmi->heightModel;
  switch (hm) {
  case CesiumI3S::HeightModel::Ellipsoidal:
  case CesiumI3S::HeightModel::GravityRelatedHeight:
  case CesiumI3S::HeightModel::Orthometric:
    return hm;
  }

  errors.emplaceError("Unsupported vertical CRS: unknown HeightModel value.");
  return std::nullopt;
}

bool isDracoBuffer(std::span<const std::byte> data) {
  return data.size() >= 5 && static_cast<char>(data[0]) == 'D' &&
         static_cast<char>(data[1]) == 'R' &&
         static_cast<char>(data[2]) == 'A' &&
         static_cast<char>(data[3]) == 'C' && static_cast<char>(data[4]) == 'O';
}

} // anonymous namespace

I3SConverterResult
I3SToGltfConverter::convert(const I3SToGltfConverterInput& input) {
  I3SConverterResult result;

  // Spatial reference validation
  if (!validateHorizontalCrs(input.spatialReference, result.errors)) {
    return result;
  }

  const auto heightModelOpt =
      resolveHeightModel(input.heightModelInfo, result.errors);
  if (!heightModelOpt.has_value()) {
    return result;
  }
  const CesiumI3S::HeightModel heightModel = *heightModelOpt;

  // Decode geometry
  DecodedGeometry geometry;

  if (input.pGeometryBuffer != nullptr) {
    // Modern path (1.7+)
    if (input.pGeometryBuffer->isDraco()) {
      // GeometryBuffer says Draco — decode via Draco decoder.
      geometry = I3SDracoDecoder::decode(input.geometryBytes);
    } else if (isDracoBuffer(input.geometryBytes)) {
      // Buffer content starts with Draco magic (defensive check).
      geometry = I3SDracoDecoder::decode(input.geometryBytes);
    } else {
      geometry = I3SBinaryDecoder::decodeModern(
          input.geometryBytes,
          *input.pGeometryBuffer);
    }
  } else if (input.pDefaultSchema != nullptr) {
    // Legacy path (< 1.7)
    if (isDracoBuffer(input.geometryBytes)) {
      geometry = I3SDracoDecoder::decode(input.geometryBytes);
    } else {
      geometry = I3SBinaryDecoder::decodeLegacy(
          input.geometryBytes,
          *input.pDefaultSchema);
    }
  } else {
    result.errors.emplaceError(
        "I3SToGltfConverterInput: either pGeometryBuffer or pDefaultSchema "
        "must be non-null.");
    return result;
  }

  // Propagate decode errors/warnings.
  result.errors.merge(geometry.errors);
  if (!geometry.errors.errors.empty()) {
    return result;
  }

  if (geometry.positions.empty()) {
    // Empty geometry is not an error; return an empty result.
    return result;
  }

  // Coordinate transform + UV crop
  I3SCoordinateTransform::transform(
      geometry,
      input.nodeCenterLonLatDegHeight,
      input.nodeObbRotation,
      heightModel,
      input.pGeoidGrid);

  // Assemble glTF
  result.model = I3SGltfAssembler::assemble(
      geometry,
      input.pMaterialDef,
      input.textureUri);

  return result;
}

} // namespace CesiumI3SContent
