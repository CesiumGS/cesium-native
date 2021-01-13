#include "CesiumAsync/CacheAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CacheAssetRequest.h"

namespace CesiumAsync {
	CacheAssetAccessor::CacheAssetAccessor(
		std::unique_ptr<IAssetAccessor> assetAccessor,
		std::unique_ptr<ICacheDatabase> cacheDatabase) 
		: _pAssetAccessor{ std::move(assetAccessor) }
		, _pCacheDatabase{std::move(cacheDatabase)}
	{
	}

	CacheAssetAccessor::~CacheAssetAccessor() noexcept {}

	std::unique_ptr<IAssetRequest> CacheAssetAccessor::requestAsset(const AsyncSystem* asyncSystem, const std::string& url, const std::vector<THeader>& headers) {
		return std::make_unique<CacheAssetRequest>(url, 
			headers, 
			asyncSystem, 
			this->_pAssetAccessor.get(), 
			this->_pCacheDatabase.get());
	}

	void CacheAssetAccessor::tick() noexcept {
		_pAssetAccessor->tick();
	}
}