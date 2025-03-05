// Heavily inspired by PntsToGltfConverter.cpp
#include "BatchTableToGltfStructuralMetadata.h"

#include <Cesium3DTilesContent/BinaryToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverterUtility.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesContent/I3dmToGltfConverter.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/AttributeCompression.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <glm/detail/setup.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/matrix.hpp>
#include <rapidjson/document.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGltf;

namespace Cesium3DTilesContent {
using namespace GltfConverterUtility;

namespace {
struct I3dmHeader {
  unsigned char magic[4] = {0, 0, 0, 0};
  uint32_t version = 0;
  uint32_t byteLength = 0;
  uint32_t featureTableJsonByteLength = 0;
  uint32_t featureTableBinaryByteLength = 0;
  uint32_t batchTableJsonByteLength = 0;
  uint32_t batchTableBinaryByteLength = 0;
  uint32_t gltfFormat = 0;
};

struct DecodedInstances {
  std::vector<glm::vec3> positions;
  std::vector<glm::fquat> rotations;
  std::vector<glm::vec3> scales;
  std::string gltfUri;
  bool rotationENU = false;
  std::optional<glm::dvec3> rtcCenter;
};

// Instance positions may arrive in ECEF coordinates or with other large
// displacements that will cause problems during rendering. Determine the mean
// position of the instances and reposition them relative to it, thus creating
// a new RTC center.
//
// If an RTC center value is already present, then the newly-computed center is
// added to it.

void repositionInstances(DecodedInstances& decodedInstances) {
  if (decodedInstances.positions.empty()) {
    return;
  }
  glm::dvec3 newCenter(0.0, 0.0, 0.0);
  for (const auto& pos : decodedInstances.positions) {
    newCenter += glm::dvec3(pos);
  }
  newCenter /= static_cast<double>(decodedInstances.positions.size());
  std::transform(
      decodedInstances.positions.begin(),
      decodedInstances.positions.end(),
      decodedInstances.positions.begin(),
      [&](const glm::vec3& pos) {
        return glm::vec3(glm::dvec3(pos) - newCenter);
      });
  if (decodedInstances.rtcCenter) {
    newCenter += *decodedInstances.rtcCenter;
  }
  decodedInstances.rtcCenter = newCenter;
}

void parseI3dmHeader(
    const std::span<const std::byte>& instancesBinary,
    I3dmHeader& header,
    uint32_t& headerLength,
    GltfConverterResult& result) {
  if (instancesBinary.size() < sizeof(I3dmHeader)) {
    result.errors.emplaceError("The I3DM is invalid because it is too small to "
                               "include a I3DM header.");
    return;
  }

  const I3dmHeader* pHeader =
      reinterpret_cast<const I3dmHeader*>(instancesBinary.data());

  header = *pHeader;
  headerLength = sizeof(I3dmHeader);

  if (pHeader->version != 1) {
    result.errors.emplaceError(fmt::format(
        "The I3DM file is version {}, which is unsupported.",
        pHeader->version));
    return;
  }

  if (static_cast<uint32_t>(instancesBinary.size()) < pHeader->byteLength) {
    result.errors.emplaceError(
        "The I3DM is invalid because the total data available is less than the "
        "size specified in its header.");
    return;
  }
}

struct I3dmContent {
  uint32_t instancesLength = 0;
  std::optional<glm::dvec3> rtcCenter;
  std::optional<glm::dvec3> quantizedVolumeOffset;
  std::optional<glm::dvec3> quantizedVolumeScale;
  bool eastNorthUp = false;

  // Offsets into the feature table.
  std::optional<uint32_t> position;
  std::optional<uint32_t> positionQuantized;
  std::optional<uint32_t> normalUp;
  std::optional<uint32_t> normalRight;
  std::optional<uint32_t> normalUpOct32p;
  std::optional<uint32_t> normalRightOct32p;
  std::optional<uint32_t> scale;
  std::optional<uint32_t> scaleNonUniform;
  std::optional<uint32_t> batchId;
  CesiumUtility::ErrorList errors;
};

glm::vec3 decodeOct32P(const uint16_t rawOct[2]) {
  glm::dvec3 result = CesiumUtility::AttributeCompression::octDecodeInRange(
      rawOct[0],
      rawOct[1],
      static_cast<uint16_t>(65535));
  return glm::vec3(result);
}

/*
  Calculate the rotation quaternion described by the up, right vectors passed
  in NORMAL_UP and NORMAL_RIGHT.

  There may be a faster method that avoids creating a rotation matrix, but it is
  hard to get the exceptional cases correct e.g., rotations of 180 degrees about
  an axis.
 */

glm::quat rotationFromUpRight(const glm::vec3& up, const glm::vec3& right) {
  glm::vec3 forward = cross(right, up);
  glm::mat3x3 rotMat(right, up, forward);
  return glm::quat(rotMat);
}

struct ConvertedI3dm {
  GltfConverterResult gltfResult;
  DecodedInstances decodedInstances;
  // If there's a batch table, then its data need to be saved until the glTF
  // model has been processed. The feature table json and binary data need to be
  // saved too, in order to extract any batch IDs. A future optimization would
  // be to save only those batch IDs and not the whole binary data blob, thus
  // saving some transient memory use.
  //
  // It's painful to copy rapidjson::Document, and not necessary for our
  // purposes, so we allocate them on the heap and keep shared pointers.
  std::shared_ptr<rapidjson::Document> pFeatureTableJson;
  std::shared_ptr<rapidjson::Document> pBatchTableJson;
  std::vector<std::byte> featureTableBinaryData;
  std::vector<std::byte> batchTableBinaryData;
  ConvertedI3dm()
      : pFeatureTableJson(std::make_shared<rapidjson::Document>()),
        pBatchTableJson(std::make_shared<rapidjson::Document>()) {}
  ConvertedI3dm(ConvertedI3dm&&) = default;
};

/* The approach:
  + Parse the i3dm header, decoding and creating all the instance transforms.
  This includes "exotic" things like OCT encoding of rotations and ENU rotations
  for each instance.
  + For each node with a mesh (a "mesh node"), the instance transforms must be
    transformed into the local coordinates of the mesh and then stored in the
    EXT_mesh_gpu_instancing extension for that node. It would be nice to avoid a
    lot of duplicate data for mesh nodes with the same chain of transforms to
    the tile root. One way to do this would be to store the accessors in a hash
    table, hashed by mesh transform.
  + Add the instance transforms to the glTF buffers, buffer views, and
    accessors.
*/

void validateI3dmDataSections(
    const std::span<const std::byte>& instancesBinary,
    const I3dmHeader& header,
    uint32_t headerLength,
    CesiumUtility::ErrorList& errors) {
  size_t dataSectionOffset = headerLength;
  if (dataSectionOffset + header.featureTableJsonByteLength >
      instancesBinary.size()) {
    errors.emplaceError(fmt::format(
        "Invalid I3dm feature table offset {} length {} "
        "file length {}",
        dataSectionOffset,
        header.featureTableJsonByteLength,
        instancesBinary.size()));
    return;
  }
  dataSectionOffset += header.featureTableJsonByteLength;
  if (dataSectionOffset + header.featureTableBinaryByteLength >
      instancesBinary.size()) {
    errors.emplaceError(fmt::format(
        "Invalid I3dm feature table binary offset {} length {} "
        "file length {}",
        dataSectionOffset,
        header.featureTableBinaryByteLength,
        instancesBinary.size()));
    return;
  }
  dataSectionOffset += header.featureTableBinaryByteLength;
  if (dataSectionOffset + header.batchTableJsonByteLength >
      instancesBinary.size()) {
    errors.emplaceError(fmt::format(
        "Invalid I3dm batch table offset {} length {} "
        "file length {}",
        dataSectionOffset,
        header.batchTableJsonByteLength,
        instancesBinary.size()));
    return;
  }
  dataSectionOffset += header.batchTableJsonByteLength;
  if (dataSectionOffset + header.batchTableJsonByteLength >
      instancesBinary.size()) {
    errors.emplaceError(fmt::format(
        "Invalid I3dm batch table binary offset {} length {} "
        "file length {}",
        dataSectionOffset,
        header.batchTableBinaryByteLength,
        instancesBinary.size()));
    return;
  }
}

void parseJsonAndBinaryData(
    const std::span<const std::byte>& instancesBinary,
    const I3dmHeader& header,
    uint32_t headerLength,
    ConvertedI3dm& convertedI3dm,
    CesiumUtility::ErrorList& errors) {
  validateI3dmDataSections(instancesBinary, header, headerLength, errors);
  if (errors.hasErrors()) {
    return;
  }
  // Offset to the beginning of each section as it is parsed in turn.
  size_t dataSectionOffset = headerLength;
  auto featureTableJsonData = instancesBinary.subspan(
      dataSectionOffset,
      header.featureTableJsonByteLength);
  convertedI3dm.pFeatureTableJson->Parse(
      reinterpret_cast<const char*>(featureTableJsonData.data()),
      featureTableJsonData.size());
  if (convertedI3dm.pFeatureTableJson->HasParseError()) {
    errors.emplaceError(fmt::format(
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        static_cast<uint64_t>(convertedI3dm.pFeatureTableJson->GetParseError()),
        convertedI3dm.pFeatureTableJson->GetErrorOffset()));
    return;
  }
  dataSectionOffset += header.featureTableJsonByteLength;
  auto featureTableBinaryData = instancesBinary.subspan(
      dataSectionOffset,
      header.featureTableBinaryByteLength);
  convertedI3dm.featureTableBinaryData.assign(
      featureTableBinaryData.begin(),
      featureTableBinaryData.end());
  if (header.batchTableJsonByteLength > 0) {
    dataSectionOffset += header.featureTableBinaryByteLength;
    auto batchTableJsonData = instancesBinary.subspan(
        dataSectionOffset,
        header.batchTableJsonByteLength);
    convertedI3dm.pBatchTableJson->Parse(
        reinterpret_cast<const char*>(batchTableJsonData.data()),
        batchTableJsonData.size());
    if (convertedI3dm.pBatchTableJson->HasParseError()) {
      errors.emplaceError(fmt::format(
          "Error when parsing batch table JSON, error code {} at byte offset "
          "{}",
          static_cast<uint64_t>(
              convertedI3dm.pFeatureTableJson->GetParseError()),
          convertedI3dm.pFeatureTableJson->GetErrorOffset()));
      return;
    }
  }
  dataSectionOffset += header.batchTableJsonByteLength;
  auto batchTableBinaryData = instancesBinary.subspan(
      dataSectionOffset,
      header.batchTableBinaryByteLength);
  convertedI3dm.batchTableBinaryData.assign(
      batchTableBinaryData.begin(),
      batchTableBinaryData.end());
}

std::optional<I3dmContent> parseI3dmJson(
    rapidjson::Document& featureTableJson,
    CesiumUtility::ErrorList& errors) {
  I3dmContent parsedContent;
  // Global semantics
  if (std::optional<uint32_t> optInstancesLength =
          getValue<uint32_t>(featureTableJson, "INSTANCES_LENGTH")) {
    parsedContent.instancesLength = *optInstancesLength;
  } else {
    errors.emplaceError("Error parsing I3DM feature table, no valid "
                        "INSTANCES_LENGTH was found.");
    return {};
  }
  parsedContent.rtcCenter =
      parseArrayValueDVec3(featureTableJson, "RTC_CENTER");
  parsedContent.position =
      parseOffsetForSemantic(featureTableJson, "POSITION", errors);
  parsedContent.positionQuantized =
      parseOffsetForSemantic(featureTableJson, "POSITION_QUANTIZED", errors);
  if (errors.hasErrors()) {
    return {};
  }
  // I would have liked to just test !parsedContent.position, but the perfectly
  // reasonable value of 0 causes the test to be false!
  if (!(parsedContent.position.has_value() ||
        parsedContent.positionQuantized.has_value())) {
    errors.emplaceError("I3dm file contains neither POSITION nor "
                        "POSITION_QUANTIZED semantics.");
    return {};
  }
  if (parsedContent.positionQuantized.has_value()) {
    parsedContent.quantizedVolumeOffset =
        parseArrayValueDVec3(featureTableJson, "QUANTIZED_VOLUME_OFFSET");
    if (!parsedContent.quantizedVolumeOffset.has_value()) {
      errors.emplaceError(
          "Error parsing I3DM feature table, the I3dm uses quantized positions "
          "but has no valid QUANTIZED_VOLUME_OFFSET property");
      return {};
    }
    parsedContent.quantizedVolumeScale =
        parseArrayValueDVec3(featureTableJson, "QUANTIZED_VOLUME_SCALE");
    if (!parsedContent.quantizedVolumeScale.has_value()) {
      errors.emplaceError(
          "Error parsing I3DM feature table, the I3dm uses quantized positions "
          "but has no valid QUANTIZED_VOLUME_SCALE property");
      return {};
    }
  }
  if (std::optional<bool> optENU =
          getValue<bool>(featureTableJson, "EAST_NORTH_UP")) {
    parsedContent.eastNorthUp = *optENU;
  }
  parsedContent.normalUp =
      parseOffsetForSemantic(featureTableJson, "NORMAL_UP", errors);
  parsedContent.normalRight =
      parseOffsetForSemantic(featureTableJson, "NORMAL_RIGHT", errors);
  if (errors.hasErrors()) {
    return {};
  }
  if (parsedContent.normalUp.has_value() &&
      !parsedContent.normalRight.has_value()) {
    errors.emplaceError("I3dm has NORMAL_UP semantic without NORMAL_RIGHT.");
    return {};
  }
  if (!parsedContent.normalUp.has_value() &&
      parsedContent.normalRight.has_value()) {
    errors.emplaceError("I3dm has NORMAL_RIGHT semantic without NORMAL_UP.");
    return {};
  }
  parsedContent.normalUpOct32p =
      parseOffsetForSemantic(featureTableJson, "NORMAL_UP_OCT32P", errors);
  parsedContent.normalRightOct32p =
      parseOffsetForSemantic(featureTableJson, "NORMAL_RIGHT_OCT32P", errors);
  if (errors.hasErrors()) {
    return {};
  }
  if (parsedContent.normalUpOct32p.has_value() &&
      !parsedContent.normalRightOct32p.has_value()) {
    errors.emplaceError(
        "I3dm has NORMAL_UP_OCT32P semantic without NORMAL_RIGHT_OCT32P.");
    return {};
  }
  if (!parsedContent.normalUpOct32p.has_value() &&
      parsedContent.normalRightOct32p.has_value()) {
    errors.emplaceError(
        "I3dm has NORMAL_RIGHT_OCT32P semantic without NORMAL_UP_OCT32P.");
    return {};
  }
  parsedContent.scale =
      parseOffsetForSemantic(featureTableJson, "SCALE", errors);
  parsedContent.scaleNonUniform =
      parseOffsetForSemantic(featureTableJson, "SCALE_NON_UNIFORM", errors);
  parsedContent.batchId =
      parseOffsetForSemantic(featureTableJson, "BATCH_ID", errors);
  if (errors.hasErrors()) {
    return {};
  }
  return parsedContent;
}

CesiumAsync::Future<ConvertedI3dm> convertI3dmContent(
    const std::span<const std::byte>& instancesBinary,
    const I3dmHeader& header,
    uint32_t headerLength,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher,
    ConvertedI3dm& convertedI3dm) {
  DecodedInstances& decodedInstances = convertedI3dm.decodedInstances;
  auto finishEarly = [&]() {
    return assetFetcher.asyncSystem.createResolvedFuture(
        std::move(convertedI3dm));
  };
  if (header.featureTableJsonByteLength == 0 ||
      header.featureTableBinaryByteLength == 0) {
    return finishEarly();
  }
  const uint32_t gltfStart = headerLength + header.featureTableJsonByteLength +
                             header.featureTableBinaryByteLength +
                             header.batchTableJsonByteLength +
                             header.batchTableBinaryByteLength;
  const uint32_t gltfEnd = header.byteLength;
  auto gltfData = instancesBinary.subspan(gltfStart, gltfEnd - gltfStart);
  parseJsonAndBinaryData(
      instancesBinary,
      header,
      headerLength,
      convertedI3dm,
      convertedI3dm.gltfResult.errors);
  if (convertedI3dm.gltfResult.errors.hasErrors()) {
    return finishEarly();
  }
  std::optional<I3dmContent> parsedJsonResult = parseI3dmJson(
      *convertedI3dm.pFeatureTableJson,
      convertedI3dm.gltfResult.errors);
  if (!parsedJsonResult || convertedI3dm.gltfResult.errors.hasErrors()) {
    return finishEarly();
  }
  std::optional<CesiumAsync::Future<AssetFetcherResult>> assetFuture;
  I3dmContent& parsedContent = *parsedJsonResult;
  decodedInstances.rtcCenter = parsedContent.rtcCenter;
  decodedInstances.rotationENU = parsedContent.eastNorthUp;

  auto featureTableBinaryData = instancesBinary.subspan(
      headerLength + header.featureTableJsonByteLength,
      header.featureTableBinaryByteLength);
  const std::byte* const pBinaryData = featureTableBinaryData.data();
  const uint32_t numInstances = parsedContent.instancesLength;
  decodedInstances.positions.resize(numInstances, glm::vec3(0.0f, 0.0f, 0.0f));
  if (parsedContent.position.has_value()) {
    std::span<const glm::vec3> rawPositions(
        reinterpret_cast<const glm::vec3*>(
            pBinaryData + *parsedContent.position),
        numInstances);
    decodedInstances.positions.assign(rawPositions.begin(), rawPositions.end());
  } else if (parsedContent.positionQuantized) {
    std::span<const uint16_t[3]> rawQuantizedPositions(
        reinterpret_cast<const uint16_t(*)[3]>(
            pBinaryData + *parsedContent.positionQuantized),
        numInstances);
    std::transform(
        rawQuantizedPositions.begin(),
        rawQuantizedPositions.end(),
        decodedInstances.positions.begin(),
        [&parsedContent](auto&& posQuantized) {
          glm::vec3 position;
          for (glm::length_t j = 0; j < 3; ++j) {
            position[j] = static_cast<float>(
                posQuantized[j] / 65535.0 *
                    (*parsedContent.quantizedVolumeScale)[j] +
                (*parsedContent.quantizedVolumeOffset)[j]);
          }
          return position;
        });
  } else {
    parsedContent.errors.emplaceError(
        "Missing position or positionQuantized in parsed content");
  }

  decodedInstances.rotations.resize(
      numInstances,
      glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
  if (parsedContent.normalUp.has_value() &&
      parsedContent.normalRight.has_value()) {
    std::span<const glm::vec3> rawUp(
        reinterpret_cast<const glm::vec3*>(
            pBinaryData + *parsedContent.normalUp),
        numInstances);
    std::span<const glm::vec3> rawRight(
        reinterpret_cast<const glm::vec3*>(
            pBinaryData + *parsedContent.normalRight),
        numInstances);
    std::transform(
        rawUp.begin(),
        rawUp.end(),
        rawRight.begin(),
        decodedInstances.rotations.begin(),
        rotationFromUpRight);

  } else if (
      parsedContent.normalUpOct32p.has_value() &&
      parsedContent.normalRightOct32p.has_value()) {

    std::span<const uint16_t[2]> rawUpOct(
        reinterpret_cast<const uint16_t(*)[2]>(
            pBinaryData + *parsedContent.normalUpOct32p),
        numInstances);
    std::span<const uint16_t[2]> rawRightOct(
        reinterpret_cast<const uint16_t(*)[2]>(
            pBinaryData + *parsedContent.normalRightOct32p),
        numInstances);
    std::transform(
        rawUpOct.begin(),
        rawUpOct.end(),
        rawRightOct.begin(),
        decodedInstances.rotations.begin(),
        [](auto&& upOct, auto&& rightOct) {
          return rotationFromUpRight(
              decodeOct32P(upOct),
              decodeOct32P(rightOct));
        });
  } else if (decodedInstances.rotationENU) {
    glm::dmat4 worldTransform = assetFetcher.tileTransform;
    if (decodedInstances.rtcCenter) {
      worldTransform = translate(worldTransform, *decodedInstances.rtcCenter);
    }
    auto worldTransformInv = inverse(worldTransform);
    std::transform(
        decodedInstances.positions.begin(),
        decodedInstances.positions.end(),
        decodedInstances.rotations.begin(),
        [&](const glm::vec3& position) {
          // Find the ENU transform using global coordinates.
          glm::dvec4 worldPos = worldTransform * glm::dvec4(position, 1.0);
          CesiumGeospatial::LocalHorizontalCoordinateSystem enu(
              (glm::dvec3(worldPos)));
          const glm::dmat4& ecef = enu.getLocalToEcefTransformation();
          // Express the rotation in the tile's coordinate system, just like
          // explicit i3dm instance rotations.
          glm::dmat4 tileFrame = worldTransformInv * ecef;
          return rotationFromUpRight(
              glm::vec3(tileFrame[1]),
              glm::vec3(tileFrame[0]));
        });
  }
  decodedInstances.scales.resize(numInstances, glm::vec3(1.0, 1.0, 1.0));
  if (parsedContent.scale.has_value()) {
    std::span<const float> rawScales(
        reinterpret_cast<const float*>(pBinaryData + *parsedContent.scale),
        numInstances);
    std::transform(
        rawScales.begin(),
        rawScales.end(),
        decodedInstances.scales.begin(),
        [](float scaleVal) { return glm::vec3(scaleVal); });
  }
  if (parsedContent.scaleNonUniform.has_value()) {
    std::span<const glm::vec3> rawScalesNonUniform(
        reinterpret_cast<const glm::vec3*>(
            pBinaryData + *parsedContent.scaleNonUniform),
        numInstances);
    std::transform(
        decodedInstances.scales.begin(),
        decodedInstances.scales.end(),
        rawScalesNonUniform.begin(),
        decodedInstances.scales.begin(),
        [](auto&& scaleUniform, auto&& scaleNonUniform) {
          return scaleUniform * scaleNonUniform;
        });
  }
  repositionInstances(decodedInstances);
  std::string baseUri;
  if (header.gltfFormat == 0) {
    // The spec says that the URL can be padded with ' ' (0x20) characters in
    // order to make the size of the whole i3dm file 8-byte aligned.
    auto rLastNotSpace =
        std::find_if_not(gltfData.rbegin(), gltfData.rend(), [](auto&& b) {
          return b == std::byte(' ');
        });
    auto spaceDistance = static_cast<size_t>(gltfData.rend() - rLastNotSpace);
    std::string gltfUri(
        reinterpret_cast<const char*>(gltfData.data()),
        spaceDistance);
    baseUri = CesiumUtility::Uri::resolve(assetFetcher.baseUrl, gltfUri);
  } else {
    baseUri = assetFetcher.baseUrl;
  }

  auto getGltf = [&]() -> CesiumAsync::Future<GltfConverterResult> {
    if (header.gltfFormat == 0) {
      // Recursively fetch and read the glTF content.
      return assetFetcher.get(baseUri).thenImmediately(
          [options, assetFetcher](AssetFetcherResult&& assetFetcherResult)
              -> CesiumAsync::Future<GltfConverterResult> {
            if (assetFetcherResult.errorList.hasErrors()) {
              GltfConverterResult errorResult;
              errorResult.errors.merge(assetFetcherResult.errorList);
              return assetFetcher.asyncSystem.createResolvedFuture(
                  std::move(errorResult));
            }
            return BinaryToGltfConverter::convert(
                assetFetcherResult.bytes,
                options,
                assetFetcher);
          });
    } else {
      return BinaryToGltfConverter::convert(gltfData, options, assetFetcher);
    }
  };

  return getGltf()
      .thenImmediately([options, assetFetcher, baseUri](
                           GltfConverterResult&& converterResult) {
        if (converterResult.model.has_value()) {
          CesiumGltfReader::GltfReaderResult readerResult{
              std::move(converterResult.model),
              {},
              {}};
          CesiumAsync::HttpHeaders externalRequestHeaders(
              assetFetcher.requestHeaders.begin(),
              assetFetcher.requestHeaders.end());
          return CesiumGltfReader::GltfReader::resolveExternalData(
              assetFetcher.asyncSystem,
              baseUri,
              externalRequestHeaders,
              assetFetcher.pAssetAccessor,
              options,
              std::move(readerResult));
        }
        return assetFetcher.asyncSystem.createResolvedFuture(
            CesiumGltfReader::GltfReaderResult{
                std::nullopt,
                std::move(converterResult.errors.errors),
                {}});
      })
      .thenImmediately(
          [convertedI3dm = std::move(convertedI3dm)](
              CesiumGltfReader::GltfReaderResult&& readerResult) mutable {
            if (readerResult.model)
              convertedI3dm.gltfResult.model = std::move(readerResult.model);
            CesiumUtility::ErrorList resolvedExternalErrors{
                std::move(readerResult.errors),
                {}};
            convertedI3dm.gltfResult.errors.merge(resolvedExternalErrors);
            return std::move(convertedI3dm);
          });
}

glm::dmat4
composeInstanceTransform(size_t i, const DecodedInstances& decodedInstances) {
  glm::dmat4 result(1.0);
  if (!decodedInstances.positions.empty()) {
    result = translate(result, glm::dvec3(decodedInstances.positions[i]));
  }
  if (!decodedInstances.rotations.empty()) {
    result = result * glm::mat4_cast(glm::dquat(decodedInstances.rotations[i]));
  }
  if (!decodedInstances.scales.empty()) {
    result = scale(result, glm::dvec3(decodedInstances.scales[i]));
  }
  return result;
}

std::vector<glm::dmat4> getMeshGpuInstancingTransforms(
    const Model& model,
    const ExtensionExtMeshGpuInstancing& gpuExt,
    CesiumUtility::ErrorList& errors) {
  std::vector<glm::dmat4> instances;
  if (gpuExt.attributes.empty()) {
    return instances;
  }
  auto getInstanceAccessor = [&](const char* name) -> const Accessor* {
    if (auto accessorItr = gpuExt.attributes.find(name);
        accessorItr != gpuExt.attributes.end()) {
      return Model::getSafe(&model.accessors, accessorItr->second);
    }
    return nullptr;
  };
  auto errorOut = [&](const std::string& errorMsg) {
    errors.emplaceError(errorMsg);
    return instances;
  };

  const Accessor* translations = getInstanceAccessor("TRANSLATION");
  const Accessor* rotations = getInstanceAccessor("ROTATION");
  const Accessor* scales = getInstanceAccessor("SCALE");
  int64_t count = 0;
  if (translations) {
    count = translations->count;
  }
  if (rotations) {
    if (count == 0) {
      count = rotations->count;
    } else if (count != rotations->count) {
      return errorOut(fmt::format(
          "instance rotation count {} not consistent with {}",
          rotations->count,
          count));
    }
  }
  if (scales) {
    if (count == 0) {
      count = scales->count;
    } else if (count != scales->count) {
      return errorOut(fmt::format(
          "instance scale count {} not consistent with {}",
          scales->count,
          count));
    }
  }
  if (count == 0) {
    return errorOut("No valid instance data");
  }
  instances.resize(static_cast<size_t>(count), glm::dmat4(1.0));
  if (translations) {
    AccessorView<CesiumGltf::AccessorTypes::VEC3<float>> translationAccessor(
        model,
        *translations);
    if (translationAccessor.status() == AccessorViewStatus::Valid) {
      for (unsigned i = 0; i < count; ++i) {
        auto transVec = toGlm<glm::dvec3>(translationAccessor[i]);
        instances[i] = glm::translate(instances[i], transVec);
      }
    } else {
      return errorOut("invalid accessor for instance translations");
    }
  }
  if (rotations) {
    auto quatAccessorView = getQuaternionAccessorView(model, rotations);
    std::visit(
        [&](auto&& arg) {
          for (unsigned i = 0; i < count; ++i) {
            auto quat = toGlmQuat<glm::dquat>(arg[i]);
            instances[i] = instances[i] * glm::mat4_cast(quat);
          }
        },
        quatAccessorView);
  }
  if (scales) {
    AccessorView<CesiumGltf::AccessorTypes::VEC3<float>> scaleAccessor(
        model,
        *scales);
    if (scaleAccessor.status() == AccessorViewStatus::Valid) {
      for (unsigned i = 0; i < count; ++i) {
        auto scaleFactors = toGlm<glm::dvec3>(scaleAccessor[i]);
        instances[i] = glm::scale(instances[i], scaleFactors);
      }
    } else {
      return errorOut("invalid accessor for instance translations");
    }
  }
  return instances;
}

const size_t rotOffset = sizeof(glm::vec3);
const size_t scaleOffset = rotOffset + sizeof(glm::quat);
const size_t totalStride =
    sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(glm::vec3);

void copyInstanceTransformToBuffer(
    const glm::dvec3& position,
    const glm::dquat& rotation,
    const glm::dvec3& scale,
    std::byte* pBufferLoc) {
  glm::vec3 fposition(position);
  std::memcpy(pBufferLoc, &fposition, sizeof(fposition));
  glm::quat frotation(rotation);
  std::memcpy(pBufferLoc + rotOffset, &frotation, sizeof(frotation));
  glm::vec3 fscale(scale);
  std::memcpy(pBufferLoc + scaleOffset, &fscale, sizeof(fscale));
}

void copyInstanceTransformToBuffer(
    const glm::dvec3& position,
    const glm::dquat& rotation,
    const glm::dvec3& scale,
    std::byte* pBufferData,
    size_t i) {
  copyInstanceTransformToBuffer(
      position,
      rotation,
      scale,
      pBufferData + (i * totalStride));
}

bool copyInstanceTransformToBuffer(
    const glm::dmat4& instanceTransform,
    std::byte* pBufferData,
    size_t i) {
  bool result = true;
  glm::dvec3 position, scale, skew;
  glm::dquat rotation;
  glm::dvec4 perspective;
  if (!decompose(
          instanceTransform,
          scale,
          rotation,
          position,
          skew,
          perspective)) {
    position = glm::dvec3(0.0);
    rotation = glm::dquat(1.0, 0.0, 0.0, 0.0);
    scale = glm::dvec3(1.0);
    result = false;
  }
  copyInstanceTransformToBuffer(position, rotation, scale, pBufferData, i);
  return result;
}

struct GltfAccessorCreator {
  Model& gltf;
  const int32_t instanceBufferViewId;
  const uint32_t numInstances;

  int32_t create(
      int32_t componentType,
      const std::string& type,
      uint32_t bufferOffset) {
    int32_t accessorId = createAccessorInGltf(
        gltf,
        instanceBufferViewId,
        componentType,
        numInstances,
        type);
    gltf.accessors[static_cast<uint32_t>(accessorId)].byteOffset =
        static_cast<int64_t>(bufferOffset);
    return accessorId;
  }

  int32_t create(const std::string& type, uint32_t bufferOffset) {
    return create(Accessor::ComponentType::FLOAT, type, bufferOffset);
  }

  void
  addAccessors(ExtensionExtMeshGpuInstancing& gpuExt, uint32_t dataBaseOffset) {
    gpuExt.attributes["TRANSLATION"] =
        create(Accessor::Type::VEC3, dataBaseOffset);
    gpuExt.attributes["ROTATION"] =
        create(Accessor::Type::VEC4, dataBaseOffset + rotOffset);
    gpuExt.attributes["SCALE"] =
        create(Accessor::Type::VEC3, dataBaseOffset + scaleOffset);
  }
};

// Split out the very rare case of instancing a model that already has instances
void instantiateWithExistingInstances(
    GltfConverterResult& result,
    const DecodedInstances& decodedInstances,
    int32_t instanceBufferId,
    int32_t instanceBufferViewId) {
  CESIUM_ASSERT(result.model);
  auto& instanceBuffer =
      result.model->buffers[static_cast<uint32_t>(instanceBufferId)];
  const auto numI3dmInstances =
      static_cast<uint32_t>(decodedInstances.positions.size());
  auto upToZ = CesiumGltfContent::GltfUtilities::applyGltfUpAxisTransform(
      *result.model,
      glm::dmat4x4(1.0));
  std::set<CesiumGltf::Node*> meshNodes;
  result.model->forEachPrimitiveInScene(
      -1,
      [&](Model& gltf,
          Node& node,
          Mesh&,
          MeshPrimitive&,
          const glm::dmat4& transform) {
        auto [nodeItr, inserted] = meshNodes.insert(&node);
        if (!inserted) {
          return;
        }
        std::vector<glm::dmat4> modelInstanceTransforms;
        auto& gpuExt = node.addExtension<ExtensionExtMeshGpuInstancing>();
        modelInstanceTransforms = getMeshGpuInstancingTransforms(
            *result.model,
            gpuExt,
            result.errors);
        if (numI3dmInstances * modelInstanceTransforms.size() >
            std::numeric_limits<uint32_t>::max()) {
          result.errors.emplaceError(fmt::format(
              "Too many instances: {} from i3dm and {} from glb",
              numI3dmInstances,
              modelInstanceTransforms.size()));
          return;
        }
        if (modelInstanceTransforms.empty()) {
          modelInstanceTransforms.emplace_back(1.0);
        }
        const uint32_t numNewInstances = static_cast<uint32_t>(
            numI3dmInstances * modelInstanceTransforms.size());
        const size_t instanceDataSize = totalStride * numNewInstances;
        auto dataBaseOffset =
            static_cast<uint32_t>(instanceBuffer.cesium.data.size());
        instanceBuffer.cesium.data.resize(dataBaseOffset + instanceDataSize);
        // Transform instance transform into local glTF coordinate system.
        const glm::dmat4 toTile = upToZ * transform;
        const glm::dmat4 toTileInv = inverse(toTile);
        size_t destInstanceIndx = 0;
        for (uint32_t i = 0; i < numI3dmInstances; ++i) {
          const glm::dmat4 instanceTransform =
              toTileInv * composeInstanceTransform(i, decodedInstances) *
              toTile;
          for (const auto& modelInstanceTransform : modelInstanceTransforms) {
            glm::dmat4 finalTransform =
                instanceTransform * modelInstanceTransform;
            if (!copyInstanceTransformToBuffer(
                    finalTransform,
                    &instanceBuffer.cesium.data[dataBaseOffset],
                    destInstanceIndx++)) {
              result.errors.emplaceWarning(
                  "Matrix decompose failed. Default identity values copied to "
                  "instance buffer.");
            }
          }
        }
        GltfAccessorCreator accessorCreator{
            gltf,
            instanceBufferViewId,
            numNewInstances};
        accessorCreator.addAccessors(gpuExt, dataBaseOffset);
      });
}

void instantiateGltfInstances(
    GltfConverterResult& result,
    const DecodedInstances& decodedInstances) {
  assert(result.model.has_value());
  std::set<CesiumGltf::Node*> meshNodes;
  int32_t instanceBufferId = createBufferInGltf(*result.model);
  auto& instanceBuffer =
      result.model->buffers[static_cast<uint32_t>(instanceBufferId)];
  int32_t instanceBufferViewId = createBufferViewInGltf(
      *result.model,
      instanceBufferId,
      0,
      static_cast<int64_t>(totalStride));
  const auto numInstances =
      static_cast<uint32_t>(decodedInstances.positions.size());
  if (result.model->isExtensionUsed(
          ExtensionExtMeshGpuInstancing::ExtensionName)) {
    instantiateWithExistingInstances(
        result,
        decodedInstances,
        instanceBufferId,
        instanceBufferViewId);
  } else {
    auto upToZ = CesiumGltfContent::GltfUtilities::applyGltfUpAxisTransform(
        *result.model,
        glm::dmat4x4(1.0));
    result.model->forEachPrimitiveInScene(
        -1,
        [&](Model& gltf,
            Node& node,
            Mesh&,
            MeshPrimitive&,
            const glm::dmat4& transform) {
          auto [nodeItr, inserted] = meshNodes.insert(&node);
          if (!inserted) {
            return;
          }
          auto& gpuExt = node.addExtension<ExtensionExtMeshGpuInstancing>();
          gltf.addExtensionRequired(
              ExtensionExtMeshGpuInstancing::ExtensionName);
          const size_t instanceDataSize = totalStride * numInstances;
          auto dataBaseOffset =
              static_cast<uint32_t>(instanceBuffer.cesium.data.size());
          instanceBuffer.cesium.data.resize(dataBaseOffset + instanceDataSize);
          // Transform instance transform into local glTF coordinate system.
          const glm::dmat4 toTile = upToZ * transform;
          const glm::dmat4 toTileInv = inverse(toTile);
          size_t destInstanceIndx = 0;
          for (unsigned i = 0; i < numInstances; ++i) {
            const glm::dmat4 instanceTransform =
                toTileInv * composeInstanceTransform(i, decodedInstances) *
                toTile;
            if (!copyInstanceTransformToBuffer(
                    instanceTransform,
                    &instanceBuffer.cesium.data[dataBaseOffset],
                    destInstanceIndx++)) {
              result.errors.emplaceWarning(
                  "Matrix decompose failed. Default identity values copied to "
                  "instance buffer.");
            }
          }
          GltfAccessorCreator accessorCreator{
              gltf,
              instanceBufferViewId,
              numInstances};
          accessorCreator.addAccessors(gpuExt, dataBaseOffset);
        });
  }
  if (decodedInstances.rtcCenter) {
    applyRtcToNodes(*result.model, *decodedInstances.rtcCenter);
  }
  instanceBuffer.byteLength =
      static_cast<int64_t>(instanceBuffer.cesium.data.size());
  result.model->bufferViews[static_cast<uint32_t>(instanceBufferViewId)]
      .byteLength = instanceBuffer.byteLength;
}
} // namespace

CesiumAsync::Future<GltfConverterResult> I3dmToGltfConverter::convert(
    const std::span<const std::byte>& instancesBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  ConvertedI3dm convertedI3dm;
  I3dmHeader header;
  uint32_t headerLength = 0;
  parseI3dmHeader(
      instancesBinary,
      header,
      headerLength,
      convertedI3dm.gltfResult);
  if (convertedI3dm.gltfResult.errors) {
    return assetFetcher.asyncSystem.createResolvedFuture(
        std::move(convertedI3dm.gltfResult));
  }
  return convertI3dmContent(
             instancesBinary,
             header,
             headerLength,
             options,
             assetFetcher,
             convertedI3dm)
      .thenImmediately([](ConvertedI3dm&& convertedI3dm) {
        if (convertedI3dm.gltfResult.model) {
          instantiateGltfInstances(
              convertedI3dm.gltfResult,
              convertedI3dm.decodedInstances);
          if (!convertedI3dm.pBatchTableJson->IsObject() ||
              convertedI3dm.pBatchTableJson->HasParseError()) {
            return convertedI3dm.gltfResult;
          }
          CesiumUtility::ErrorList batchTableErrors =
              BatchTableToGltfStructuralMetadata::convertFromI3dm(
                  *convertedI3dm.pFeatureTableJson,
                  *convertedI3dm.pBatchTableJson,
                  convertedI3dm.featureTableBinaryData,
                  convertedI3dm.batchTableBinaryData,
                  *convertedI3dm.gltfResult.model);
          convertedI3dm.gltfResult.errors.merge(batchTableErrors);
        }
        return convertedI3dm.gltfResult;
      });
}
} // namespace Cesium3DTilesContent
