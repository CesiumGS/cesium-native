#include "FileAssetRequest.h"

#include "FileAssetResponse.h"
#include "Cesium3DTilesTestUtils.h"

#include "Cesium3DTiles/spdlog-cesium.h"

#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"

#include <string>
#include <fstream>
#include <iomanip>
#include <functional>
#include <filesystem>

namespace Cesium3DTilesTests
{
	FileAssetRequest::FileAssetRequest(const std::string& url, const std::vector<CesiumAsync::IAssetAccessor::THeader>& /*headers*/) noexcept :
		_url(url),
		_pResponse(nullptr),
		_callback() 
	{
		SPDLOG_TRACE("Created FileAssetRequest with {0}", url);

		uint16_t statusCode = 200;
		std::vector<uint8_t> contents;
		std::ifstream stream(url, std::ios::in | std::ios::binary);
		if (!stream.is_open()) {
			SPDLOG_ERROR("Failed to open file {0}", url);
			statusCode = 404;
		} else {
			contents = std::vector<uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
		}

		// TODO: The content type is ONLY determined by the file extension here! 
		// This may be sufficient for now, but in the future, it may have to 
		// examine the contents.
		std::string actualExtension = std::filesystem::path(url).extension().string();
		std::vector<std::string> extensions{ "json", "b3dm", "cmpt", "glTF" };
		for (const std::string& extension : extensions) {
			if (actualExtension == extension) {
				std::string contentType = extension;
				_pResponse = std::make_unique<FileAssetResponse>(url, statusCode, contentType, contents);
			}
		}
		if (!_pResponse) {
			SPDLOG_ERROR("Unkonwn content type for {0}", url);
		}
	}

	std::string FileAssetRequest::url() const noexcept {
		return _url;
	}

	CesiumAsync::IAssetResponse* FileAssetRequest::response() noexcept {
		return this->_pResponse.get();
	}

	void FileAssetRequest::bind(std::function<void(IAssetRequest*)> callback) noexcept {
		this->_callback = callback;
		if (callback) {
			callback(this);
		}
	}

	void FileAssetRequest::cancel() noexcept {
		// Nothing to do here
	}

}

