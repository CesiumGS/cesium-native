#include "FileAssetResponse.h"

#include "Cesium3DTiles/spdlog-cesium.h"

#include <gsl/span>

#include <string>
#include <vector>

namespace Cesium3DTilesTests
{
	FileAssetResponse::FileAssetResponse(
		const std::string& fileName,
		const std::string& contentType,
		const std::vector<uint8_t>& fileData) : 
		_fileName(fileName),
		_contentType(contentType),
		_fileData(fileData) // TODO: Yeah, will be copied. However...
	{
		SPDLOG_TRACE("Created FileAssetResponse for {0} data from {1}", contentType, fileName);
	}

	uint16_t FileAssetResponse::statusCode() const {
		// TODO: Some optimistic assumptions are made here...
		return 200;
	}

	std::string FileAssetResponse::contentType() const {
		return _contentType;
	}

	gsl::span<const uint8_t> FileAssetResponse::data() const {
		return gsl::span<const uint8_t>(_fileData.data(), _fileData.size());
	}

}