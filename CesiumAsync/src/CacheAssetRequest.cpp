#include "CacheAssetRequest.h"
#include <cassert>

namespace CesiumAsync {
	CacheAssetRequest::CacheAssetRequest(const std::string& url, 
		const std::vector<IAssetAccessor::THeader>& headers, 
		const AsyncSystem* pAsyncSystem,
		IAssetAccessor* pAssetAccessor, 
		ICacheDatabase* pCacheDatabase)
		: _headers{ headers }
		, _url{ url } 
		, _pAsyncSystem{ pAsyncSystem }
		, _pAssetAccessor{ pAssetAccessor }
		, _pCacheDatabase{ pCacheDatabase }
	{
	}

	const std::string& CacheAssetRequest::method() const
	{
		if (this->_cacheItem) {
			return this->_cacheItem->cacheRequest().method();
		}

		return this->_pRequest->method();
	}

	const std::string& CacheAssetRequest::url() const
	{
		if (this->_cacheItem) {
			return this->_cacheItem->cacheRequest().url();
		}

		return this->_pRequest->url();
	}

	const std::map<std::string, std::string>& CacheAssetRequest::headers() const
	{
		if (this->_cacheItem) {
			return this->_cacheItem->cacheRequest().headers();
		}

		return this->_pRequest->headers();
	}

	const IAssetResponse* CacheAssetRequest::response() const
	{
		if (this->_cacheAssetResponse) {
			return &this->_cacheAssetResponse.value();
		}

		return this->_pRequest->response();
	}

	void CacheAssetRequest::bind(std::function<void(IAssetRequest*)> callback)
	{
		this->_pRequest = nullptr;
		this->_cacheItem = std::nullopt;
		this->_cacheAssetResponse = std::nullopt;

		std::shared_ptr<IAssetRequest> pRequest = std::shared_ptr<IAssetRequest>(this->_pAssetAccessor->requestAsset(this->_pAsyncSystem, this->_url, this->_headers));
		if (!pRequest) {
			return;
		}

		// retrieve from cache
		std::string hash = hashRequest(*pRequest);
		ICacheDatabase* pCacheDatabase = this->_pCacheDatabase;
		this->_pAsyncSystem->runInWorkerThread([hash, pCacheDatabase]() -> std::optional<CacheItem> {
			std::string error;
			std::optional<CacheItem> cacheItem;
			if (pCacheDatabase->getEntry(hash, cacheItem, error)) {
				if (cacheItem) {
					if (isCacheValid(*cacheItem)) {
						return cacheItem;
					}
					else {
						error.clear();
						if (!pCacheDatabase->removeEntry(hash, error)) {
							// TODO: log error
							return std::nullopt;
						}
					}
				}
			}
			else {
				// TODO: log error
				return std::nullopt;
			}

			return std::nullopt;
		}).thenInMainThread([this, callback, hash, pRequest](std::optional<CacheItem> cacheItem) mutable {
			if (cacheItem) {
				this->_cacheAssetResponse = CacheAssetResponse(&this->_cacheItem.value());
				callback(this);
				return;
			}

			// cache doesn't have it or the cache item is stale, so request it
			ICacheDatabase* pCacheDatabase = this->_pCacheDatabase;
			const AsyncSystem* pAsyncSystem = this->_pAsyncSystem;
			pRequest->bind([callback, hash, pCacheDatabase, pAsyncSystem](IAssetRequest* pCompletedRequest) {
				if (!pCompletedRequest) {
					callback(pCompletedRequest);
					return;
				}

				if (shouldCacheRequest(*pCompletedRequest)) {
					//pAsyncSystem->runInWorkerThread([pRequest, pCacheDatabase, hash]() {
					//	std::string error;
					//	if (!pCacheDatabase->storeResponse(hash,
					//		calculateExpiryTime(*pRequest),
					//		*pRequest,
					//		error))
					//	{
					//		// TODO: log error here
					//	}
					//});
				}

				callback(pCompletedRequest);
			});

			this->_pRequest = pRequest;
		});

		//this->_pAsyncSystem->runInWorkerThread([this, callback = std::move(callback), pRequest = std::move(pRequest)]() mutable {
		//	std::string hash = hashRequest(*pRequest);
		//	std::string error;
		//	if (this->_pCacheDatabase->getEntry(hash, this->_cacheItem, error)) {
		//		if (this->_cacheItem) {
		//			if (isCacheValid(*this->_cacheItem)) {
		//				this->_cacheAssetResponse = CacheAssetResponse(&this->_cacheItem.value());
		//				callback(this);
		//				return;
		//			}
		//			else {
		//				error.clear();
		//				if (!this->_pCacheDatabase->removeEntry(hash, error)) {
		//					// TODO: log error
		//				}
		//			}
		//		}
		//	}
		//	else {
		//		// TODO: log error
		//	}

		//	// cache doesn't have it or the cache item is stale, so request it
		//	ICacheDatabase* pCacheDatabase = this->_pCacheDatabase;
		//	pRequest->bind([callback, hash, pCacheDatabase](IAssetRequest* pCompletedRequest) {
		//		if (!pCompletedRequest) {
		//			callback(pCompletedRequest);
		//			return;
		//		}

		//		if (shouldCacheRequest(*pCompletedRequest)) {
		//			std::string error;
		//			if(!pCacheDatabase->storeResponse(hash, 
		//					calculateExpiryTime(*pCompletedRequest), 
		//					*pCompletedRequest, 
		//					error)) 
		//			{ 
		//				// TODO: log error here
		//			}
		//		}

		//		callback(pCompletedRequest);
		//	});

		//	this->_pRequest = std::move(pRequest);
		//});
	}

	void CacheAssetRequest::cancel() noexcept
	{
		if (this->_pRequest) {
			this->_pRequest->cancel();
		}
	}

	/*static*/ bool CacheAssetRequest::isCacheValid(const CacheItem& /*cacheItem*/) {
		return true;
	}

	/*static*/ bool CacheAssetRequest::shouldCacheRequest(const IAssetRequest& /*request*/) {
		return true;
	}

	/*static*/ std::time_t CacheAssetRequest::calculateExpiryTime(const IAssetRequest& /*request*/) {
		return std::time(0);
	}

	/*static*/ std::string CacheAssetRequest::hashRequest(const IAssetRequest& request) {
		return request.url();
	}
}