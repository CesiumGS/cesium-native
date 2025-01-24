#include "BatchTableToGltfStructuralMetadata.h"

#include <Cesium3DTilesContent/B3dmToGltfConverter.h>
#include <Cesium3DTilesContent/BinaryToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace Cesium3DTilesContent {
namespace {
struct B3dmHeader {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJsonByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJsonByteLength;
  uint32_t batchTableBinaryByteLength;
};

struct B3dmHeaderLegacy1 {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t batchLength;
  uint32_t batchTableByteLength;
};

struct B3dmHeaderLegacy2 {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t batchTableJsonByteLength;
  uint32_t batchTableBinaryByteLength;
  uint32_t batchLength;
};

void parseB3dmHeader(
    const std::span<const std::byte>& b3dmBinary,
    B3dmHeader& header,
    uint32_t& headerLength,
    GltfConverterResult& result) {
  if (b3dmBinary.size() < sizeof(B3dmHeader)) {
    result.errors.emplaceError("The B3DM is invalid because it is too small to "
                               "include a B3DM header.");
    return;
  }

  const B3dmHeader* pHeader =
      reinterpret_cast<const B3dmHeader*>(b3dmBinary.data());

  header = *pHeader;
  headerLength = sizeof(B3dmHeader);

  // Legacy header #1: [batchLength] [batchTableByteLength]
  // Legacy header #2: [batchTableJsonByteLength] [batchTableBinaryByteLength]
  // [batchLength] Current header: [featureTableJsonByteLength]
  // [featureTableBinaryByteLength] [batchTableJsonByteLength]
  // [batchTableBinaryByteLength] If the header is in the first legacy format
  // 'batchTableJsonByteLength' will be the start of the JSON string (a
  // quotation mark) or the glTF magic. Accordingly its first byte will be
  // either 0x22 or 0x67, and so the minimum uint32 expected is 0x22000000 =
  // 570425344 = 570MB. It is unlikely that the feature table JSON will exceed
  // this length. The check for the second legacy format is similar, except it
  // checks 'batchTableBinaryByteLength' instead
  if (pHeader->batchTableJsonByteLength >= 570425344) {
    // First legacy check
    headerLength = sizeof(B3dmHeaderLegacy1);
    const B3dmHeaderLegacy1* pLegacy1 =
        reinterpret_cast<const B3dmHeaderLegacy1*>(b3dmBinary.data());
    header.batchTableJsonByteLength = pLegacy1->batchTableByteLength;
    header.batchTableBinaryByteLength = 0;
    header.featureTableJsonByteLength = 0;
    header.featureTableBinaryByteLength = 0;

    result.errors.emplaceWarning(
        "This b3dm header is using the legacy "
        "format[batchLength][batchTableByteLength]. "
        "The new format "
        "is[featureTableJsonByteLength][featureTableBinaryByteLength]["
        "batchTableJsonByteLength][batchTableBinaryByteLength] "
        "from "
        "https://github.com/CesiumGS/3d-tiles/tree/master/specification/"
        "TileFormats/Batched3DModel.");
  } else if (pHeader->batchTableBinaryByteLength >= 570425344) {
    // Second legacy check
    headerLength = sizeof(B3dmHeaderLegacy2);
    const B3dmHeaderLegacy2* pLegacy2 =
        reinterpret_cast<const B3dmHeaderLegacy2*>(b3dmBinary.data());
    header.batchTableJsonByteLength = pLegacy2->batchTableJsonByteLength;
    header.batchTableBinaryByteLength = pLegacy2->batchTableBinaryByteLength;
    header.featureTableJsonByteLength = 0;
    header.featureTableBinaryByteLength = 0;

    result.errors.emplaceWarning(
        "This b3dm header is using the legacy format "
        "[batchTableJsonByteLength] [batchTableBinaryByteLength] "
        "[batchLength]. "
        "The new format is [featureTableJsonByteLength] "
        "[featureTableBinaryByteLength] [batchTableJsonByteLength] "
        "[batchTableBinaryByteLength] "
        "from "
        "https://github.com/CesiumGS/3d-tiles/tree/master/specification/"
        "TileFormats/Batched3DModel.");
  }

  if (static_cast<uint32_t>(b3dmBinary.size()) < pHeader->byteLength) {
    result.errors.emplaceError(
        "The B3DM is invalid because the total data available is less than the "
        "size specified in its header.");
    return;
  }
}

CesiumAsync::Future<GltfConverterResult> convertB3dmContentToGltf(
    const std::span<const std::byte>& b3dmBinary,
    const B3dmHeader& header,
    uint32_t headerLength,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  const uint32_t glbStart = headerLength + header.featureTableJsonByteLength +
                            header.featureTableBinaryByteLength +
                            header.batchTableJsonByteLength +
                            header.batchTableBinaryByteLength;
  const uint32_t glbEnd = header.byteLength;

  if (glbEnd <= glbStart) {
    GltfConverterResult result;
    result.errors.emplaceError(
        "The B3DM is invalid because the start of the "
        "glTF model is after the end of the entire B3DM.");
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  const std::span<const std::byte> glbData =
      b3dmBinary.subspan(glbStart, glbEnd - glbStart);

  return BinaryToGltfConverter::convert(glbData, options, assetFetcher);
}

rapidjson::Document parseFeatureTableJsonData(
    const std::span<const std::byte>& featureTableJsonData,
    GltfConverterResult& result) {
  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(featureTableJsonData.data()),
      featureTableJsonData.size());
  if (document.HasParseError()) {
    result.errors.emplaceError(fmt::format(
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset()));
    return document;
  }

  const auto rtcIt = document.FindMember("RTC_CENTER");
  if (rtcIt != document.MemberEnd() && rtcIt->value.IsArray() &&
      rtcIt->value.Size() == 3 && rtcIt->value[0].IsNumber() &&
      rtcIt->value[1].IsNumber() && rtcIt->value[2].IsNumber()) {
    CESIUM_ASSERT(result.model.has_value());
    // Add the RTC_CENTER value to the glTF as a CESIUM_RTC extension.
    rapidjson::Value& rtcValue = rtcIt->value;
    auto& cesiumRTC =
        result.model->addExtension<CesiumGltf::ExtensionCesiumRTC>();
    result.model->addExtensionRequired(
        CesiumGltf::ExtensionCesiumRTC::ExtensionName);
    cesiumRTC.center = {
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble()};
  }

  return document;
}

void convertB3dmMetadataToGltfStructuralMetadata(
    const std::span<const std::byte>& b3dmBinary,
    const B3dmHeader& header,
    uint32_t headerLength,
    GltfConverterResult& result) {
  if (result.model && header.featureTableJsonByteLength > 0) {
    CesiumGltf::Model& gltf = result.model.value();

    const std::span<const std::byte> featureTableJsonData =
        b3dmBinary.subspan(headerLength, header.featureTableJsonByteLength);
    rapidjson::Document featureTableJson =
        parseFeatureTableJsonData(featureTableJsonData, result);

    const int64_t batchTableStart = headerLength +
                                    header.featureTableJsonByteLength +
                                    header.featureTableBinaryByteLength;
    const int64_t batchTableLength =
        header.batchTableBinaryByteLength + header.batchTableJsonByteLength;

    if (batchTableLength > 0) {
      const std::span<const std::byte> batchTableJsonData = b3dmBinary.subspan(
          static_cast<size_t>(batchTableStart),
          header.batchTableJsonByteLength);
      const std::span<const std::byte> batchTableBinaryData =
          b3dmBinary.subspan(
              static_cast<size_t>(
                  batchTableStart + header.batchTableJsonByteLength),
              header.batchTableBinaryByteLength);

      rapidjson::Document batchTableJson;
      batchTableJson.Parse(
          reinterpret_cast<const char*>(batchTableJsonData.data()),
          batchTableJsonData.size());
      if (batchTableJson.HasParseError()) {
        result.errors.emplaceWarning(fmt::format(
            "Error when parsing batch table JSON, error code {} at byte "
            "offset "
            "{}. Skip parsing metadata",
            batchTableJson.GetParseError(),
            batchTableJson.GetErrorOffset()));
        return;
      }

      // upgrade batch table to glTF structural metadata and append the result
      result.errors.merge(BatchTableToGltfStructuralMetadata::convertFromB3dm(
          featureTableJson,
          batchTableJson,
          batchTableBinaryData,
          gltf));
    }
  }
}
} // namespace

CesiumAsync::Future<GltfConverterResult> B3dmToGltfConverter::convert(
    const std::span<const std::byte>& b3dmBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  GltfConverterResult result;
  B3dmHeader header;
  uint32_t headerLength = 0;
  parseB3dmHeader(b3dmBinary, header, headerLength, result);
  if (result.errors) {
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  return convertB3dmContentToGltf(
             b3dmBinary,
             header,
             headerLength,
             options,
             assetFetcher)
      .thenImmediately(
          [b3dmBinary, header, headerLength](GltfConverterResult&& glbResult) {
            if (!glbResult.errors) {
              convertB3dmMetadataToGltfStructuralMetadata(
                  b3dmBinary,
                  header,
                  headerLength,
                  glbResult);
            }
            return std::move(glbResult);
          });
}
} // namespace Cesium3DTilesContent
