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
    return TileRenderContentLoadResult{{}, TileLoadState::FailedTemporarily};
  }

  if (loadInfo.contentOptions.generateMissingNormalsSmooth) {
    gltfResult.model->generateMissingNormalsSmooth();
  }

  TileRenderContentLoadResult result;
  result.content.model = std::move(gltfResult.model);
  result.state = TileLoadState::ContentLoaded;
  return result;
}
} // namespace

bool TileRenderContentLoader::canCreateRenderContent(
    const std::string& tileUrl,
    const gsl::span<const std::byte>& tileContentBinary) {
  return GltfConverters::canConvertContent(tileUrl, tileContentBinary);
}

CesiumAsync::Future<TileRenderContentLoadResult> TileRenderContentLoader::load(
    const std::shared_ptr<CesiumAsync::IAssetRequest>& completedTileRequest,
    const TileContentLoadInfo& loadInfo) {
  auto& asyncSystem = loadInfo.asyncSystem;
  const auto& assetAccessor = loadInfo.pAssetAccessor;

  auto response = completedTileRequest->response();
  if (!response) {
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{{}, TileLoadState::FailedTemporarily});
  }

  uint16_t statusCode = response->statusCode();
  if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{{}, TileLoadState::FailedTemporarily});
  }

  // Convert to gltf
  const std::string& tileUrl = completedTileRequest->url();
  CesiumGltfReader::GltfReaderOptions gltfOptions;
  gltfOptions.ktx2TranscodeTargets =
      loadInfo.contentOptions.ktx2TranscodeTargets;
  GltfConverterResult result =
      GltfConverters::convert(tileUrl, response->data(), gltfOptions);

  // Report any errors if there are any
  logErrorsAndWarnings(loadInfo.pLogger, tileUrl, result.errors);
  if (result.errors) {
    return asyncSystem.createResolvedFuture<TileRenderContentLoadResult>(
        TileRenderContentLoadResult{{}, TileLoadState::FailedTemporarily});
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
      .thenImmediately([loadInfo, completedTileRequest](
                           CesiumGltfReader::GltfReaderResult&& gltfResult) {
        return postProcessGltf(
            completedTileRequest,
            loadInfo,
            std::move(gltfResult));
      });
}
} // namespace Cesium3DTilesSelection
