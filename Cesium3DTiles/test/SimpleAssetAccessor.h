#pragma once

#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include <memory>
#include <map>

class SimpleAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
	SimpleAssetAccessor(std::map<std::string, std::unique_ptr<SimpleAssetRequest>> mockCompletedRequests)
		: mockCompletedRequests{std::move(mockCompletedRequests)}
	{}

	virtual std::unique_ptr<CesiumAsync::IAssetRequest> requestAsset(
		const std::string& url,
		const std::vector<THeader>&) override
	{
		auto mockRequestIt = mockCompletedRequests.find(url);
		if (mockRequestIt != mockCompletedRequests.end()) {
			return std::move(mockRequestIt->second);
		}

		return nullptr;
	}

	virtual void tick() noexcept override {}

	std::map<std::string, std::unique_ptr<SimpleAssetRequest>> mockCompletedRequests;
};
