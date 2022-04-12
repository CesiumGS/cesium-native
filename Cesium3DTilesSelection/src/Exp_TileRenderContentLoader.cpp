#include <Cesium3DTilesSelection/Exp_TileRenderContentLoader.h>
#include <Cesium3DTilesSelection/Exp_GltfConverters.h>

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
    const TileContentLoadInput& loadInput,
    CesiumGltfReader::GltfReaderResult&& gltfResult)
{
  const std::string& tileUrl = loadInput.pRequest->url();
  logErrors(loadInput.pLogger, tileUrl, gltfResult.errors);
  logWarnings(loadInput.pLogger, tileUrl, gltfResult.warnings);
  if (!gltfResult.errors.empty()) {
    return TileRenderContentLoadResult{{}, TileLoadState::FailedTemporarily};
  }

  if (loadInput.contentOptions.generateMissingNormalsSmooth) {

  }

  TileRenderContentLoadResult result;
  result.content.model = std::move(gltfResult.model);
  result.content.renderResources = nullptr;
  result.state = TileLoadState::ContentLoaded;
  return result;
}
} // namespace

bool TileRenderContentLoader::canCreateRenderContent(
    const std::string& tileUrl,
    const gsl::span<const std::byte>& tileContentBinary) {
  return GltfConverters::canConvertContent(tileUrl, tileContentBinary);
}

CesiumAsync::Future<TileRenderContentLoadResult>
TileRenderContentLoader::load(TileContentLoadInput&& loadInput) {
  auto& asyncSystem = loadInput.asyncSystem;
  const auto& completedTileRequest = loadInput.pRequest;
  const auto& assetAccessor = loadInput.pAssetAccessor;

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
      loadInput.contentOptions.ktx2TranscodeTargets;
  GltfConverterResult result =
      GltfConverters::convert(tileUrl, response->data(), gltfOptions);

  // Report any errors if there are any
  logErrorsAndWarnings(loadInput.pLogger, tileUrl, result.errors);
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
      .thenImmediately([originalInput = std::move(loadInput)](
                           CesiumGltfReader::GltfReaderResult&& gltfResult) {
        return postProcessGltf(originalInput, std::move(gltfResult));
      });
}
} // namespace Cesium3DTilesSelection
