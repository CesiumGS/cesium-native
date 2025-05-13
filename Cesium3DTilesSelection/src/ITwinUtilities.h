#pragma once

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/ErrorList.h>

namespace Cesium3DTilesSelection {
void parseITwinErrorResponseIntoErrorList(
    const CesiumAsync::IAssetResponse& response,
    CesiumUtility::ErrorList& errors);
}