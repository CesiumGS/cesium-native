#pragma once

#include <Cesium3DTilesSelection/ErrorList.h>

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace Cesium3DTilesSelection {
void logTileLoadResult(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const ErrorList& errorLists);
}
