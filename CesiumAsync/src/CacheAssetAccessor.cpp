#include "CesiumAsync/CacheAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/CacheItem.h"

namespace CesiumAsync {
	class CacheAssetResponse : public IAssetResponse {
    public:
        CacheAssetResponse(const CacheItem* pCacheItem) 
			: _pCacheItem{pCacheItem}
		{}

		virtual uint16_t statusCode() const override { 
			return this->_pCacheItem->cacheResponse().statusCode(); 
		}

		virtual const std::string& contentType() const override {
			return this->_pCacheItem->cacheResponse().contentType(); 
		}

		virtual const std::map<std::string, std::string>& headers() const override {
			return this->_pCacheItem->cacheResponse().headers(); 
		}

		virtual const ResponseCacheControl* cacheControl() const override {
			return this->_pCacheItem->cacheResponse().cacheControl(); 
		}

		virtual gsl::span<const uint8_t> data() const override {
			return this->_pCacheItem->cacheResponse().data(); 
		}

    private:
        const CacheItem* _pCacheItem;
	};

	class CacheAssetRequest : public IAssetRequest 
	{
	public:
        CacheAssetRequest(CacheItem cacheItem) 
			: _cacheItem{std::move(cacheItem)}
		{
			_response = std::make_optional<CacheAssetResponse>(&this->_cacheItem.value());
		}

		virtual const std::string& method() const override {
			return this->_cacheItem->cacheRequest().method();
		}

		virtual const std::string& url() const override {
			return this->_cacheItem->cacheRequest().url();
		}

		virtual const std::map<std::string, std::string>& headers() const override {
			return this->_cacheItem->cacheRequest().headers();
		}

		virtual const IAssetResponse* response() const override {
			return &_response.value();
		}

	private:
		std::optional<CacheAssetResponse> _response;
        std::optional<CacheItem> _cacheItem;
	};

	CacheAssetAccessor::CacheAssetAccessor(
		std::unique_ptr<IAssetAccessor> assetAccessor,
		std::unique_ptr<ICacheDatabase> cacheDatabase) 
		: _pAssetAccessor{ std::move(assetAccessor) }
		, _pCacheDatabase{std::move(cacheDatabase)}
	{
	}

	CacheAssetAccessor::~CacheAssetAccessor() noexcept {}

	void CacheAssetAccessor::requestAsset(const AsyncSystem* pAsyncSystem, 
		const std::string& url, 
		const std::vector<THeader>& headers,
		std::function<void(std::unique_ptr<IAssetRequest>)> callback) 
	{
		struct Receiver {
			std::unique_ptr<IAssetRequest> pCompletedRequest;
		};

		pAsyncSystem->runInWorkerThread([this, url]() -> std::optional<CacheItem> {
			std::string error;
			std::optional<CacheItem> cacheItem;
			if (!this->_pCacheDatabase->getEntry(url, cacheItem, error)) {
				return std::nullopt;
			}

			return cacheItem;
		}).thenInMainThread([this, pAsyncSystem, url, headers, callback](std::optional<CacheItem> cacheItem) {
			if (cacheItem) {
				callback(std::make_unique<CacheAssetRequest>(cacheItem.value()));
				return;
			}

			ICacheDatabase* pCacheDatabase = this->_pCacheDatabase.get();
			this->_pAssetAccessor->requestAsset(pAsyncSystem, url, headers, [pAsyncSystem, pCacheDatabase, callback](std::unique_ptr<IAssetRequest> pCompletedRequest) {
				// cache doesn't have it or the cache item is stale, so request it
				if (shouldCacheRequest(*pCompletedRequest)) {
					std::shared_ptr<Receiver> pReceiver = std::make_shared<Receiver>();
					pReceiver->pCompletedRequest = std::move(pCompletedRequest);
					pAsyncSystem->runInWorkerThread([pCacheDatabase, pReceiver]() -> std::shared_ptr<Receiver> {
						std::string error;
						if(!pCacheDatabase->storeResponse(hashRequest(*pReceiver->pCompletedRequest), 
								calculateExpiryTime(*pReceiver->pCompletedRequest), 
								*pReceiver->pCompletedRequest, 
								error)) 
						{ 
							// TODO: log error here
						}

						return pReceiver;
					}).thenInMainThread([callback](std::shared_ptr<Receiver> pReceiver) {
						callback(std::move(pReceiver->pCompletedRequest));
					});

					return;
				}

				callback(std::move(pCompletedRequest));
			});
		});
	}

	void CacheAssetAccessor::tick() noexcept {
		_pAssetAccessor->tick();
	}

	/*static*/ bool CacheAssetAccessor::isCacheValid(const CacheItem& /*cacheItem*/) {
		return true;
	}

	/*static*/ bool CacheAssetAccessor::shouldCacheRequest(const IAssetRequest& /*request*/) {
		return true;
	}

	/*static*/ std::time_t CacheAssetAccessor::calculateExpiryTime(const IAssetRequest& /*request*/) {
		return std::time(0);
	}

	/*static*/ std::string CacheAssetAccessor::hashRequest(const IAssetRequest& request) {
		return request.url();
	}
}