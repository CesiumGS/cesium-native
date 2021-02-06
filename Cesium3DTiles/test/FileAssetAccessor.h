#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"

#include <memory>

namespace Cesium3DTilesTests
{
	/**
	 * @brief Implementation of an IAssetAccessor that accesses files
	 */
	class FileAssetAccessor : public CesiumAsync::IAssetAccessor
	{
	public:
		std::unique_ptr<CesiumAsync::IAssetRequest> requestAsset(
			const std::string& url,
			const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
		) override;

		void tick() noexcept override;
	};
}
