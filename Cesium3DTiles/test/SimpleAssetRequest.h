#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "SimpleAssetResponse.h"

class SimpleAssetRequest : public CesiumAsync::IAssetRequest {
public:
	SimpleAssetRequest(const std::string& url,
		std::unique_ptr<SimpleAssetResponse> response)
		: requestUrl{ url }
		, pResponse{ std::move(response) }
	{}

	virtual std::string url() const override {
		return this->requestUrl;
	}

	virtual CesiumAsync::IAssetResponse* response() override {
		return this->pResponse.get();
	}

	virtual void bind(std::function<void(CesiumAsync::IAssetRequest*)> callback) override {
		callback(this);
	}

	virtual void cancel() noexcept override {}

	std::string requestUrl;
	std::unique_ptr<SimpleAssetResponse> pResponse;
};
