#include "Batched3DModelContent.h"

#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "upgradeBatchTableToFeatureMetadata.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltf/ExtensionModelExtFeatureMetadata.h>
#include <CesiumUtility/Tracing.h>

#include <rapidjson/document.h>

#include <cstddef>
#include <stdexcept>

namespace Cesium3DTilesSelection {

using namespace CesiumAsync;

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

namespace {

rapidjson::Document parseFeatureTableJsonData(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const gsl::span<const std::byte>& featureTableJsonData) {
  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(featureTableJsonData.data()),
      featureTableJsonData.size());
  if (document.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset());
    return document;
  }

  const auto rtcIt = document.FindMember("RTC_CENTER");
  if (rtcIt != document.MemberEnd() && rtcIt->value.IsArray() &&
      rtcIt->value.Size() == 3 && rtcIt->value[0].IsNumber() &&
      rtcIt->value[1].IsNumber() && rtcIt->value[2].IsNumber()) {
    // Add the RTC_CENTER value to the glTF itself.
    rapidjson::Value& rtcValue = rtcIt->value;
    gltf.extras["RTC_CENTER"] = {
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble()};
  }

  return document;
}

} // namespace

Future<std::unique_ptr<TileContentLoadResult>>
Batched3DModelContent::load(const TileContentLoadInput& input) {
  const AsyncSystem& asyncSystem = input.asyncSystem;
  const std::shared_ptr<spdlog::logger>& pLogger = input.pLogger;
  const std::shared_ptr<IAssetRequest>& pRequest = input.pRequest;
  const std::string& url = pRequest->url();
  const HttpHeaders& headers = pRequest->headers();
  const std::shared_ptr<IAssetAccessor>& pAssetAccessor = input.pAssetAccessor;
  const gsl::span<const std::byte> data = pRequest->response()->data();

  // TODO: actually use the b3dm payload
  if (data.size() < sizeof(B3dmHeader)) {
    throw std::runtime_error("The B3DM is invalid because it is too small to "
                             "include a B3DM header.");
  }

  CESIUM_TRACE("Cesium3DTilesSelection::Batched3DModelContent::load");
  const B3dmHeader* pHeader = reinterpret_cast<const B3dmHeader*>(data.data());

  B3dmHeader header = *pHeader;
  uint32_t headerLength = sizeof(B3dmHeader);
  // uint32_t batchLength;

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
        reinterpret_cast<const B3dmHeaderLegacy1*>(data.data());
    // batchLength = pLegacy1->batchLength;
    header.batchTableJsonByteLength = pLegacy1->batchTableByteLength;
    header.batchTableBinaryByteLength = 0;
    header.featureTableJsonByteLength = 0;
    header.featureTableBinaryByteLength = 0;

    SPDLOG_LOGGER_WARN(
        pLogger,
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
        reinterpret_cast<const B3dmHeaderLegacy2*>(data.data());
    // batchLength = pLegacy2->batchLength;
    header.batchTableJsonByteLength = pLegacy2->batchTableJsonByteLength;
    header.batchTableBinaryByteLength = pLegacy2->batchTableBinaryByteLength;
    header.featureTableJsonByteLength = 0;
    header.featureTableBinaryByteLength = 0;

    SPDLOG_LOGGER_WARN(
        pLogger,
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

  if (static_cast<uint32_t>(data.size()) < pHeader->byteLength) {
    throw std::runtime_error(
        "The B3DM is invalid because the total data available is less than the "
        "size specified in its header.");
  }

  const uint32_t glbStart = headerLength + header.featureTableJsonByteLength +
                            header.featureTableBinaryByteLength +
                            header.batchTableJsonByteLength +
                            header.batchTableBinaryByteLength;
  const uint32_t glbEnd = header.byteLength;

  if (glbEnd <= glbStart) {
    throw std::runtime_error("The B3DM is invalid because the start of the "
                             "glTF model is after the end of the entire B3DM.");
  }

  const gsl::span<const std::byte> glbData =
      data.subspan(glbStart, glbEnd - glbStart);

  return GltfContent::load(
             asyncSystem,
             pLogger,
             url,
             headers,
             pAssetAccessor,
             glbData)
      .thenInWorkerThread([header = std::move(header),
                           headerLength,
                           pLogger,
                           pRequest](std::unique_ptr<TileContentLoadResult>&&
                                         pResult) {
        if (pResult->model && header.featureTableJsonByteLength > 0) {
          CesiumGltf::Model& gltf = pResult->model.value();

          const gsl::span<const std::byte> data = pRequest->response()->data();
          const gsl::span<const std::byte> featureTableJsonData =
              data.subspan(headerLength, header.featureTableJsonByteLength);
          rapidjson::Document featureTable =
              parseFeatureTableJsonData(pLogger, gltf, featureTableJsonData);

          const int64_t batchTableStart = headerLength +
                                          header.featureTableJsonByteLength +
                                          header.featureTableBinaryByteLength;
          const int64_t batchTableLength = header.batchTableBinaryByteLength +
                                           header.batchTableJsonByteLength;

          if (batchTableLength > 0) {
            const gsl::span<const std::byte> batchTableJsonData = data.subspan(
                static_cast<size_t>(batchTableStart),
                header.batchTableJsonByteLength);
            const gsl::span<const std::byte> batchTableBinaryData =
                data.subspan(
                    static_cast<size_t>(
                        batchTableStart + header.batchTableJsonByteLength),
                    header.batchTableBinaryByteLength);

            rapidjson::Document batchTableJson;
            batchTableJson.Parse(
                reinterpret_cast<const char*>(batchTableJsonData.data()),
                batchTableJsonData.size());
            if (batchTableJson.HasParseError()) {
              SPDLOG_LOGGER_WARN(
                  pLogger,
                  "Error when parsing batch table JSON, error code {} at byte "
                  "offset "
                  "{}. Skip parsing metadata",
                  batchTableJson.GetParseError(),
                  batchTableJson.GetErrorOffset());
              return std::move(pResult);
            }

            upgradeBatchTableToFeatureMetadata(
                pLogger,
                gltf,
                featureTable,
                batchTableJson,
                batchTableBinaryData);
          }
        }

        return std::move(pResult);
      });
}

} // namespace Cesium3DTilesSelection
