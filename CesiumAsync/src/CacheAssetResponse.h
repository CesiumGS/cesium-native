#pragma once

#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/CacheItem.h"

namespace CesiumAsync {
	class CacheAssetResponse : public IAssetResponse {
    public:
        CacheAssetResponse(const CacheItem* pCacheItem);

        virtual uint16_t statusCode() const override;

        virtual const std::string& contentType() const override;

        virtual const std::map<std::string, std::string>& headers() const override;

        virtual const ResponseCacheControl* cacheControl() const override;

        virtual gsl::span<const uint8_t> data() const override;

    private:
        const CacheItem* _pCacheItem;
	};
}
