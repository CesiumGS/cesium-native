#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"

#include <string>
#include <vector>
#include <functional>

namespace Cesium3DTilesTests
{
	/**
	 * @brief Implementation of an IAssetRequest that is backed by a file
	 */
	class FileAssetRequest : public CesiumAsync::IAssetRequest {
	public:

		/**
		 * @brief Creates a new instance
		 * 
		 * @param url The URL (file path)
		 * @param headers The headers
		 */
		FileAssetRequest(const std::string& url, const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) noexcept;

		std::string url() const noexcept override;

		CesiumAsync::IAssetResponse* response() noexcept override;

		void bind(std::function<void(CesiumAsync::IAssetRequest*)> callback) noexcept override;

		void cancel() noexcept override;

	private:
		std::string _url;
		std::unique_ptr<CesiumAsync::IAssetResponse> _pResponse;
		std::function<void(CesiumAsync::IAssetRequest*)> _callback;
	};
}