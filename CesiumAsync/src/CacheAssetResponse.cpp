#include "CacheAssetResponse.h"

namespace CesiumAsync {
	CacheAssetResponse::CacheAssetResponse(const CacheItem* pCacheItem)
		: _pCacheItem{pCacheItem}
	{
	}

	uint16_t CacheAssetResponse::statusCode() const
	{
		return this->_pCacheItem->cacheResponse().statusCode();
	}

	const std::string& CacheAssetResponse::contentType() const
	{
		return this->_pCacheItem->cacheResponse().contentType();
	}

	const std::map<std::string, std::string>& CacheAssetResponse::headers() const
	{
		return this->_pCacheItem->cacheResponse().headers();
	}

	const ResponseCacheControl* CacheAssetResponse::cacheControl() const
	{
		return this->_pCacheItem->cacheResponse().cacheControl();
	}

	gsl::span<const uint8_t> CacheAssetResponse::data() const
	{
		return this->_pCacheItem->cacheResponse().data();
	}
}