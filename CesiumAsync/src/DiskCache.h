#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "sqlite3.h"
#include <memory>
#include <optional>
#include <string>
#include <map>

namespace CesiumAsync {
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