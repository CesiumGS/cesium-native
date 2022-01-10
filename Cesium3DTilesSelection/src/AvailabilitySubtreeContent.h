#pragma once

#include "Cesium3DTilesSelection/Library.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/Future.h"
#include "CesiumAsync/HttpHeaders.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeometry/Availability.h"

#include <memory>

namespace Cesium3DTilesSelection {

class CESIUM3DTILESSELECTION_API AvailabilitySubtreeContent final {
public:
  static CesiumAsync::Future<
      std::unique_ptr<CesiumGeometry::AvailabilitySubtree>>
  load(
      CesiumAsync::AsyncSystem asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& url,
      const gsl::span<const std::byte>& data,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const CesiumAsync::HttpHeaders& headers);
};

} // namespace Cesium3DTilesSelection
