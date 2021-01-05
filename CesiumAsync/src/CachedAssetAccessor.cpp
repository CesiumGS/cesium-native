#include "CesiumAsync/CachedAssetAccessor.h"
#include "sqlite3.h"

namespace CesiumAsync {
	struct CachedAssetAccessor::Impl {
		std::shared_ptr<IAssetAccessor> pAssetAccessor;
	};

	CachedAssetAccessor::CachedAssetAccessor(std::shared_ptr<IAssetAccessor> pAssetAccessor) {
		_impl = std::make_unique<Impl>();
		_impl->pAssetAccessor = pAssetAccessor;
	}

	CachedAssetAccessor::~CachedAssetAccessor() {}

	std::unique_ptr<IAssetRequest> CachedAssetAccessor::requestAsset(const std::string& url, const std::vector<THeader>& headers) {
		return _impl->pAssetAccessor->requestAsset(url, headers);
	}

	void CachedAssetAccessor::tick() noexcept {
		_impl->pAssetAccessor->tick();
	}
}