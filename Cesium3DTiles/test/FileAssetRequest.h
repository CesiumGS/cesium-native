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
		FileAssetRequest(const std::string& url, const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers);

		~FileAssetRequest() override;

		std::string url() const override;

		CesiumAsync::IAssetResponse* response() override;

		void bind(std::function<void(CesiumAsync::IAssetRequest*)> callback) override;

		void cancel() noexcept override;

	private:
		std::string _url;
		CesiumAsync::IAssetResponse* _pResponse;
		std::function<void(CesiumAsync::IAssetRequest*)> _callback;
	};
}