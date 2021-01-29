#include "CesiumAsync/CacheAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/CacheItem.h"
#include "InternalTimegm.h"
#include <iomanip>
#include <sstream>
#include <ctype.h>
#include <algorithm>

namespace CesiumAsync {
	class CacheAssetResponse : public IAssetResponse {
    public:
        CacheAssetResponse(const CacheItem* pCacheItem) 
			: _pCacheItem{pCacheItem}
		{}

		virtual uint16_t statusCode() const override { 
			return this->_pCacheItem->cacheResponse.statusCode; 
		}

		virtual const std::string& contentType() const override {
			return this->_pCacheItem->cacheResponse.contentType; 
		}

		virtual const HttpHeaders& headers() const override {
			return this->_pCacheItem->cacheResponse.headers; 
		}

		virtual const ResponseCacheControl* cacheControl() const override {
			return this->_pCacheItem->cacheResponse.cacheControl ? 
				&this->_pCacheItem->cacheResponse.cacheControl.value() : 
				nullptr; 
		}

		virtual gsl::span<const uint8_t> data() const override {
			return gsl::span<const uint8_t>(
				this->_pCacheItem->cacheResponse.data.data(), 
				this->_pCacheItem->cacheResponse.data.size()); 
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
			return this->_cacheItem->cacheRequest.method;
		}

		virtual const std::string& url() const override {
			return this->_cacheItem->cacheRequest.url;
		}

		virtual const HttpHeaders& headers() const override {
			return this->_cacheItem->cacheRequest.headers;
		}

		virtual const IAssetResponse* response() const override {
			return &_response.value();
		}

	private:
		std::optional<CacheAssetResponse> _response;
        std::optional<CacheItem> _cacheItem;
	};

	static std::time_t convertHttpDateToTime(const std::string& httpDate);

	static bool shouldRevalidateCache(const CacheItem& cacheItem);

	static bool isCacheStale(const CacheItem& cacheItem);

	static bool shouldCacheRequest(const IAssetRequest& request);

	static bool shouldDeleteCache(const IAssetRequest& request);

	static std::time_t calculateExpiryTime(const IAssetRequest& request);

	static std::unique_ptr<IAssetRequest> updateCacheItem(CacheItem& cacheItem, const IAssetRequest& request);

	CacheAssetAccessor::CacheAssetAccessor(
		std::unique_ptr<IAssetAccessor> assetAccessor,
		std::unique_ptr<ICacheDatabase> cacheDatabase,
		uint32_t databaseCleanCheckpoint) 
		: _databaseCleanCheckpoint{databaseCleanCheckpoint}
		, _requestCount{}
		, _pAssetAccessor{ std::move(assetAccessor) }
		, _pCacheDatabase{std::move(cacheDatabase)}
	{
	}

	CacheAssetAccessor::~CacheAssetAccessor() noexcept {}

	void CacheAssetAccessor::requestAsset(const AsyncSystem* pAsyncSystem, 
		const std::string& url, 
		const std::vector<THeader>& headers,
		std::function<void(std::shared_ptr<IAssetRequest>)> callback) 
	{
		{
			std::lock_guard<std::mutex> guard(this->_requestCountMutex);
			++this->_requestCount;
			if (this->_requestCount > 10000) {
				this->_requestCount = 0;
				pAsyncSystem->runInWorkerThread([this]() {
					std::string error;
					if (!this->_pCacheDatabase->prune(error)) {
						// TODO: log error
					}
				});
			}
		}

		pAsyncSystem->runInWorkerThread([this, pAsyncSystem, url, headers, callback]() {
			bool readError = false;
			std::string error;
			std::optional<CacheItem> cacheItem;
			auto getEntryCallback = [&cacheItem](CacheItem item) mutable -> bool { cacheItem = std::move(item); return true; };
			if (!this->_pCacheDatabase->getEntry(url, getEntryCallback, error)) {
				// TODO: log error
				readError = true;
			}
			
			// if no cache found, then request directly to the server
			if (!cacheItem) { 
				ICacheDatabase* pCacheDatabase = this->_pCacheDatabase.get();
				this->_pAssetAccessor->requestAsset(pAsyncSystem, url, headers, 
					[pAsyncSystem, pCacheDatabase, callback, readError](std::shared_ptr<IAssetRequest> pCompletedRequest) {
						if (!readError && shouldCacheRequest(*pCompletedRequest)) {
							pAsyncSystem->runInWorkerThread([pCacheDatabase, pCompletedRequest]() {
								std::string error;
								if (!pCacheDatabase->storeResponse(pCompletedRequest->url(),
									calculateExpiryTime(*pCompletedRequest),
									*pCompletedRequest,
									error))
								{
									// TODO: log error here
								}
							});
						}

						callback(pCompletedRequest);
					});

				return;
			}

			// cache is stale and need revalidation 
			if (shouldRevalidateCache(*cacheItem)) {
				std::vector<THeader> newHeaders = headers;
				const CacheResponse& cacheResponse = cacheItem->cacheResponse;
				const HttpHeaders& responseHeaders = cacheResponse.headers;
				HttpHeaders::const_iterator lastModifiedHeader = responseHeaders.find("Last-Modified");
				HttpHeaders::const_iterator etagHeader = responseHeaders.find("Etag");
				if (etagHeader != responseHeaders.end()) {
					newHeaders.emplace_back("If-None-Match", etagHeader->second);
				}
				else if (lastModifiedHeader != responseHeaders.end()) {
					newHeaders.emplace_back("If-Modified-Since", lastModifiedHeader->second);
				}

				ICacheDatabase* pCacheDatabase = this->_pCacheDatabase.get();
				this->_pAssetAccessor->requestAsset(pAsyncSystem, url, newHeaders,
					[pCacheDatabase, pAsyncSystem, callback, cacheItem = std::move(cacheItem)](std::shared_ptr<IAssetRequest> pCompletedRequest) mutable {
						if (!pCompletedRequest) {
							return;
						}

						std::shared_ptr<IAssetRequest> pRequestToStore;
						if (pCompletedRequest->response()->statusCode() == 304) { // status Not-Modified
							pRequestToStore = updateCacheItem(*cacheItem, *pCompletedRequest);
						}
						else {
							pRequestToStore = pCompletedRequest;
						}

						if (shouldCacheRequest(*pRequestToStore)) {
							pAsyncSystem->runInWorkerThread([pCacheDatabase, pRequestToStore]() {
								std::string error;
								if (!pCacheDatabase->storeResponse(pRequestToStore->url(),
									calculateExpiryTime(*pRequestToStore),
									*pRequestToStore,
									error))
								{
									// TODO: log error here
								}
							});
						}
						else if (shouldDeleteCache(*pRequestToStore)) {
							pAsyncSystem->runInWorkerThread([pCacheDatabase, pRequestToStore]() {
								std::string error;
								if (!pCacheDatabase->removeEntry(pRequestToStore->url(), error)) {
									// TODO: log error here
								}
							});
						}

						callback(pRequestToStore);
					});

				return;
			}

			pAsyncSystem->runInMainThread([cacheItem = std::move(cacheItem), callback]() mutable {
				callback(std::make_unique<CacheAssetRequest>(std::move(cacheItem.value())));
			});
		});
	}

	void CacheAssetAccessor::tick() noexcept {
		_pAssetAccessor->tick();
	}

	bool shouldRevalidateCache(const CacheItem& cacheItem) {
		const CacheResponse& cacheResponse = cacheItem.cacheResponse;
		const std::optional<ResponseCacheControl>& cacheControl = cacheResponse.cacheControl;
		if (cacheControl) {
			if (isCacheStale(cacheItem) && cacheControl->mustRevalidate()) {
				return true;
			}
		}

		return isCacheStale(cacheItem);
	}

	bool isCacheStale(const CacheItem& cacheItem) {
		const CacheResponse& cacheResponse = cacheItem.cacheResponse;
		const std::optional<ResponseCacheControl> &cacheControl = cacheResponse.cacheControl;
		std::time_t currentTime = std::time(0);
		if (cacheControl) {
			return std::difftime(cacheItem.expiryTime, currentTime) < 0.0;
		}

		const HttpHeaders& responseHeaders = cacheResponse.headers;
		HttpHeaders::const_iterator expiresHeader = responseHeaders.find("Expires");
		if (expiresHeader != responseHeaders.end()) {
			return std::difftime(convertHttpDateToTime(expiresHeader->second), currentTime) < 0.0;
		}

		// Without "Cache-Control" or "Expires" headers, we don't know if the cache is stale or not, so make it stale.
		// This code shouldn't execute given the condition to store a completed request being that the request should have "Cache-Control"
		// header or "Expires" header at the very begining
		return true;
	}

	bool shouldCacheRequest(const IAssetRequest& request) {
		// no response then no cache
		const IAssetResponse* response = request.response();
		if (!response) {
			return false;
		}

		// check if request is Authorization request, then no cache
		const HttpHeaders& requestHeaders = request.headers();
		if (requestHeaders.find("Authorization") != requestHeaders.end()) {
			return false;
		}

		// only cache GET method
		if (request.method() != "GET") {
			return false;
		}

		// check if response status code is cacheable
		uint16_t statusCode = response->statusCode();
		if (statusCode != 200 && // status OK
			statusCode != 201 && // status Created
			statusCode != 202 && // status Accepted
			statusCode != 203 && // status Non-Authoritive Information
			statusCode != 204 && // status No-Content
			statusCode != 205 && // status Reset-Content
			statusCode != 304)   // status Not-Modifed
		{
			return false;
		}

		// check if cache control contains no-store or no-cache directives
		const ResponseCacheControl* cacheControl = response->cacheControl();
		int maxAge = 0;
		if (cacheControl) {
			if (cacheControl->noStore() || cacheControl->noCache()) {
				return false;
			}

			maxAge = cacheControl->maxAge();
		}

		// check response header contains expires if maxAge is not specified 
		const HttpHeaders& responseHeaders = response->headers();
		if (maxAge == 0) {
			HttpHeaders::const_iterator expiresHeader = responseHeaders.find("Expires");
			if (expiresHeader == responseHeaders.end()) {
				return false;
			}

			return std::difftime(convertHttpDateToTime(expiresHeader->second), std::time(0)) > 0.0;
		}

		return true;
	}

	bool shouldDeleteCache(const IAssetRequest& request) {
		const IAssetResponse* response = request.response();
		if (!response || !response->cacheControl()) {
			return false;
		}

		return response->cacheControl()->noStore();
	}

	std::time_t calculateExpiryTime(const IAssetRequest& request) {
		const IAssetResponse* response = request.response();
		const ResponseCacheControl* cacheControl = response->cacheControl();
		if (cacheControl) {
			if (cacheControl->maxAge() != 0) {
				return std::time(0) + cacheControl->maxAge();
			}
		}

		const HttpHeaders& responseHeaders = response->headers();
		HttpHeaders::const_iterator expiresHeader = responseHeaders.find("Expires");
		if (expiresHeader != responseHeaders.end()) {
			return convertHttpDateToTime(expiresHeader->second);
		}

		return std::time(0);
	}

	std::unique_ptr<IAssetRequest> updateCacheItem(CacheItem& cacheItem, const IAssetRequest& request) {
		for (const std::pair<const std::string, std::string>& header : request.headers()) {
			cacheItem.cacheRequest.headers[header.first] = header.second;
		}

		const IAssetResponse* response = request.response();
		if (response) {
			for (const std::pair<const std::string, std::string>& header : response->headers()) {
				cacheItem.cacheResponse.headers[header.first] = header.second;
			}

			if (response->cacheControl()) {
				cacheItem.cacheResponse.cacheControl = *response->cacheControl();
			}
		}

		return std::make_unique<CacheAssetRequest>(std::move(cacheItem));
	}

	std::time_t convertHttpDateToTime(const std::string& httpDate) {
		std::tm tm = {};
		std::stringstream ss(httpDate);
		ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
		return internalTimegm(&tm);
	}
}