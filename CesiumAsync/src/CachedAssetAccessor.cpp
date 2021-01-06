#include "CesiumAsync/CachedAssetAccessor.h"
#include "DiskCache.h"
#include <optional>

namespace CesiumAsync {
	struct CachedAssetAccessor::Impl {
		std::shared_ptr<IAssetAccessor> assetAccessor;
		std::optional<DiskCache> cacheStorage;
	};

	CachedAssetAccessor::CachedAssetAccessor(std::shared_ptr<IAssetAccessor> assetAccessor) {
		_impl = std::make_unique<Impl>();
		_impl->assetAccessor = assetAccessor;
		_impl->cacheStorage = std::make_optional<DiskCache>("C:/Users/bao/Documents/test.db");
	}

	CachedAssetAccessor::~CachedAssetAccessor() noexcept {}

	std::unique_ptr<IAssetRequest> CachedAssetAccessor::requestAsset(const std::string& url, const std::vector<THeader>& headers) {
		return _impl->assetAccessor->requestAsset(url, headers);
	}

	void CachedAssetAccessor::tick() noexcept {
		_impl->assetAccessor->tick();
	}
}