#pragma once

#include "CesiumAsync/HttpHeaders.h"
#include <string>
#include <optional>
#include <map>

namespace CesiumAsync {
	class ResponseCacheControl {
	public:
		ResponseCacheControl(bool mustRevalidate,
			bool noCache,
			bool noStore,
			bool noTransform,
			bool accessControlPublic,
			bool accessControlPrivate,
			bool proxyRevalidate,
			int maxAge,
			int sharedMaxAge);

		inline bool mustRevalidate() const { return _mustRevalidate; }

		inline bool noCache() const { return _noCache; }

		inline bool noStore() const { return _noStore; }

		inline bool noTransform() const { return _noTransform; }

		inline bool accessControlPublic() const { return _accessControlPublic; }

		inline bool accessControlPrivate() const { return _accessControlPrivate; }

		inline bool proxyRevalidate() const { return _proxyRevalidate; }

		int maxAge() const { return _maxAge; }

		int sharedMaxAge() const { return _sharedMaxAge; }

		static std::optional<ResponseCacheControl> parseFromResponseHeaders(const HttpHeaders& headers);

	private:
		bool _mustRevalidate;
		bool _noCache;
		bool _noStore;
		bool _noTransform;
		bool _accessControlPublic;
		bool _accessControlPrivate;
		bool _proxyRevalidate;
		int _maxAge;
		int _sharedMaxAge;
	};
}
