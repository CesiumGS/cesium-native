#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "sqlite3.h"
#include <memory>
#include <optional>
#include <string>
#include <map>

namespace CesiumAsync {
	class DiskCachedAssetResponse : public IAssetResponse {
	public:
		DiskCachedAssetResponse(uint16_t statusCode, 
			const std::string& contentType, 
			std::unique_ptr<uint8_t> pData, 
			size_t dataSize) 
			: _statusCode{statusCode}
			, _contentType{contentType}
			, _pData{std::move(pData)}
			, _dataSize{dataSize}
		{}

		uint16_t statusCode() const override { return this->_statusCode; }

		std::string contentType() const override { return this->_contentType; }

		gsl::span<const uint8_t> data() const override { 
			return gsl::span<const uint8_t>(reinterpret_cast<const uint8_t *>(this->_pData.get()), this->_dataSize); 
		}

	private:
		uint16_t _statusCode;
		std::string _contentType;
		std::unique_ptr<uint8_t> _pData;
		size_t _dataSize;
	};

	class DiskCachedAssetRequest : public IAssetRequest {
	public:
		DiskCachedAssetRequest(std::unique_ptr<DiskCachedAssetResponse> pResponse, const std::string &requestUrl)
			: _pResponse{std::move(pResponse)}
			, _requestUrl{requestUrl}
		{}

		const IAssetResponse* response() const override { return this->_pResponse.get(); }

        void bind(std::function<void(IAssetRequest*)> callback) override {}

		std::string url() const override { return this->_requestUrl; }

        void cancel() noexcept override {}

	private:
		std::unique_ptr<DiskCachedAssetResponse> _pResponse;
		std::string _requestUrl;
	};

	class DiskCache {
	public:
		DiskCache(const std::string &databaseName);

		DiskCache(const DiskCache&) = delete;

		DiskCache(DiskCache&& other) noexcept;

		DiskCache& operator=(const DiskCache&) = delete;

		DiskCache& operator=(DiskCache&&) noexcept;

		~DiskCache() noexcept;

		std::unique_ptr<IAssetRequest> getEntry(const std::string& url) const;

		void insertEntry(const IAssetRequest& entry);

		void removeEntry(const std::string& url);

		void prune();

		void clearAll();

	private:
		struct Sqlite3StmtWrapper {
			Sqlite3StmtWrapper(const std::string& statement, sqlite3* connection);

			Sqlite3StmtWrapper(const Sqlite3StmtWrapper &) = delete;

			Sqlite3StmtWrapper(Sqlite3StmtWrapper &&) noexcept;

			Sqlite3StmtWrapper &operator=(const Sqlite3StmtWrapper &) = delete;

			Sqlite3StmtWrapper &operator=(Sqlite3StmtWrapper &&) noexcept;

			~Sqlite3StmtWrapper() noexcept;

			sqlite3_stmt* pStmt;
		};

		sqlite3* _pConnection;
		std::optional<Sqlite3StmtWrapper> _insertStmt;
	};
}