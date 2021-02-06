
#include "FileAssetAccessor.h"
#include "FileAssetRequest.h"

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"

#include <memory>

namespace Cesium3DTilesTests
{
	std::unique_ptr<CesiumAsync::IAssetRequest> FileAssetAccessor::requestAsset(
		const std::string& url,
		const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
	) {
		return std::make_unique<FileAssetRequest>(url, headers);
	}

	void FileAssetAccessor::tick() noexcept {
		// Does nothing
	}
}




