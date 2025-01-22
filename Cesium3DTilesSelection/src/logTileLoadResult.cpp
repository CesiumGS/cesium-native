#include "logTileLoadResult.h"

#include <CesiumUtility/ErrorList.h>

#include <fmt/format.h>
#include <spdlog/logger.h>

#include <memory>
#include <string>

using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
void logTileLoadResult(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const ErrorList& errorLists) {
  errorLists.logError(pLogger, fmt::format("Failed to load {}", url));
  errorLists.logWarning(pLogger, fmt::format("Warning when loading {}", url));
}
} // namespace Cesium3DTilesSelection
