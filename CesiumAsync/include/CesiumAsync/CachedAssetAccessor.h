#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"
#include <string>
#include <memory>

namespace CesiumAsync {
	class CachedAssetAccessor : public IAssetAccessor {
	public:
        CachedAssetAccessor(std::shared_ptr<IAssetAccessor> pAssetAccessor);

        ~CachedAssetAccessor() override;

        std::unique_ptr<IAssetRequest> requestAsset(
            const std::string& url,
            const std::vector<THeader>& headers = std::vector<THeader>()
        ) override;

        void tick() noexcept override;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
	};
}

