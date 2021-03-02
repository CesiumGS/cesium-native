#pragma once

#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include <memory>
#include <map>

class SimpleAssetAccessor : public CesiumAsync::IAssetAccessor {
public:
    SimpleAssetAccessor(std::map<std::string, std::shared_ptr<SimpleAssetRequest>>&& mockCompletedRequests)
        : mockCompletedRequests{std::move(mockCompletedRequests)}
    {}

    virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> requestAsset(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::string& url,
        const std::vector<THeader>&) override
    {
        auto mockRequestIt = mockCompletedRequests.find(url);
        if (mockRequestIt != mockCompletedRequests.end()) {
            return asyncSystem.createResolvedFuture(std::shared_ptr<CesiumAsync::IAssetRequest>(mockRequestIt->second));
        }

        return asyncSystem.createResolvedFuture(std::shared_ptr<CesiumAsync::IAssetRequest>(nullptr));
    }

    virtual void tick() noexcept override {}

    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mockCompletedRequests;
};

