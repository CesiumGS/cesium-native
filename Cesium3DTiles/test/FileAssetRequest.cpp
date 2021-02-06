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

namespace Cesium3DTilesTests
{
	FileAssetRequest::FileAssetRequest(const std::string& url, const std::vector<CesiumAsync::IAssetAccessor::THeader>& /*headers*/) :
		_url(url),
		_pResponse(nullptr),
		_callback()
	{
		SPDLOG_TRACE("Created FileAssetRequest with {0}", url);

		std::ifstream stream(url, std::ios::in | std::ios::binary);
		if (!stream.is_open()) {
			SPDLOG_ERROR("Failed to open file {0}", url);
			return;
		}
		std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

		// XXX NOTE: The content type is ONLY determined to be JSON or B3DM
		// by using the file extension here! This should examine the contents!

		if (endsWith(url, "json")) {
			std::string contentType = "json";
			_pResponse = new FileAssetResponse(url, contentType, contents);
		}
		else if (endsWith(url, "b3dm")) {
			std::string contentType = "b3dm";
			_pResponse = new FileAssetResponse(url, contentType, contents);
		}
		else {
			SPDLOG_ERROR("Unkonwn content type for {0}", url);
		}
	}

	FileAssetRequest::~FileAssetRequest() {
		delete _pResponse;
	}

	std::string FileAssetRequest::url() const {
		return _url;
	}

	CesiumAsync::IAssetResponse* FileAssetRequest::response() {
		if (this->_pResponse) {
			return this->_pResponse;
		}
		return this->_pResponse;
	}

	void FileAssetRequest::bind(std::function<void(IAssetRequest*)> callback) {
		this->_callback = callback;
		if (callback) {
			callback(this);
		}
	}

	void FileAssetRequest::cancel() noexcept {
		// Nothing to do here
	}

}

