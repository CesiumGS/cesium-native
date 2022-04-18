#include <Cesium3DTilesSelection/Exp_GltfConverters.h>
#include <Cesium3DTilesSelection/Exp_TileRenderContentLoader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/joinToString.h>

#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
void logErrors(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const std::vector<std::string>& errors) {
  if (!errors.empty()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Failed to load {}:\n- {}",
        url,
        CesiumUtility::joinToString(errors, "\n- "));
  }
}

void logWarnings(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const std::vector<std::string>& warnings) {
  if (!warnings.empty()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Warning when loading {}:\n- {}",
        url,
        CesiumUtility::joinToString(warnings, "\n- "));
  }
}

void logErrorsAndWarnings(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const ErrorList& errorLists) {
  logErrors(pLogger, url, errorLists.errors);
  logWarnings(pLogger, url, errorLists.warnings);
}

TileRenderContentLoadResult postProcessGltf(
    const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
    const TileContentLoadInfo& loadInfo,
    CesiumGltfReader::GltfReaderResult&& gltfResult) {
  const std::string& tileUrl = completedTileRequest->url();
  logErrors(loadInfo.pLogger, tileUrl, gltfResult.errors);
  logWarnings(loadInfo.pLogger, tileUrl, gltfResult.warnings);
  if (!gltfResult.model) {
    return TileRenderContentLoadResult{
        {},
        TileRenderContentFailReason::ConversionFailed};
  }

  if (loadInfo.contentOptions.generateMissingNormalsSmooth) {
    gltfResult.model->generateMissingNormalsSmooth();
  }

  TileRenderContentLoadResult result;
  result.content.model = std::move(gltfResult.model);
  result.reason = TileRenderContentFailReason::Success;
  return result;
}
} // namespace

CesiumAsync::Future<TileRenderContentLoadResult> TileRenderContentLoader::load(
    const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
    const TileContentLoadInfo& loadInfo) {
  auto& asyncSystem = loadInfo.asyncSystem;
  const auto& assetAccessor = loadInfo.pAssetAccessor;
  const std::string& tileUrl = completedTileRequest->url();

  auto pResponse = completedTileRequest->response();
  if (!pResponse) {
    SPDLOG_LOGGER_ERROR(
        loadInfo.pLogger,
        "Did not receive a valid response for tile content {}",
        tileUrl);
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{
            {},
            TileRenderContentFailReason::DataRequestFailed});
  }

  uint16_t statusCode = pResponse->statusCode();
  if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
    SPDLOG_LOGGER_ERROR(
        loadInfo.pLogger,
        "Received status code {} for tile content {}",
        statusCode,
        tileUrl);
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{
            {},
            TileRenderContentFailReason::DataRequestFailed});
  }

  // find converter
  const auto& responseData = pResponse->data();
  auto converter = GltfConverters::getConverterByMagic(responseData);
  if (!converter) {
    converter = GltfConverters::getConverterByFileExtension(tileUrl);
  }

  if (!converter) {
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{
            {},
            TileRenderContentFailReason::UnsupportedFormat});
  }

  // Convert to gltf
  CesiumGltfReader::GltfReaderOptions gltfOptions;
  gltfOptions.ktx2TranscodeTargets =
      loadInfo.contentOptions.ktx2TranscodeTargets;
  GltfConverterResult result = converter(responseData, gltfOptions);

  // Report any errors if there are any
  logErrorsAndWarnings(loadInfo.pLogger, tileUrl, result.errors);
  if (result.errors) {
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{
            {},
            TileRenderContentFailReason::ConversionFailed});
  }

  // Download any external image or buffer urls in the gltf if there are any
  CesiumGltfReader::GltfReaderResult gltfResult{
      std::move(result.model),
      {},
      {}};
  return CesiumGltfReader::GltfReader::resolveExternalData(
             asyncSystem,
             tileUrl,
             completedTileRequest->headers(),
             assetAccessor,
             gltfOptions,
             std::move(gltfResult))
      .thenInWorkerThread([loadInfo, completedTileRequest](
                              CesiumGltfReader::GltfReaderResult&& gltfResult) {
        return postProcessGltf(
            completedTileRequest,
            loadInfo,
            std::move(gltfResult));
      });
}
} // namespace Cesium3DTilesSelection
