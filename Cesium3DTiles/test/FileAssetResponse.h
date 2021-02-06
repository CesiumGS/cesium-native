#pragma once

#include "CesiumAsync/IAssetResponse.h"

#include <gsl/span>

#include <string>
#include <vector>

namespace Cesium3DTilesTests
{
	/**
	 * @brief Implementation of an IAssetResponse for data from a file.
	 */
	class FileAssetResponse : public CesiumAsync::IAssetResponse {
	public:

		/**
		 * @brief Creates a new instance.
		 * 
		 * @param fileName The file name.
		 * @param contentType The content type.
		 * @param fileData The file data. 
		 */
		FileAssetResponse(
			const std::string& fileName,
			const std::string& contentType,
			const std::vector<uint8_t>& fileData);

		uint16_t statusCode() const;

		std::string contentType() const;

		gsl::span<const uint8_t> data() const;

	private:
		std::string _fileName;
		std::string _contentType;
		std::vector<uint8_t> _fileData;
	};
}