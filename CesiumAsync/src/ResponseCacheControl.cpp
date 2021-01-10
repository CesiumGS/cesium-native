#include "CesiumAsync/ResponseCacheControl.h"
#include <map>
#include <set>

namespace CesiumAsync {
	ResponseCacheControl::ResponseCacheControl(bool mustRevalidate, 
		bool noCache, 
		bool noStore, 
		bool noTransform, 
		bool accessControlPublic, 
		bool accessControlPrivate, 
		bool proxyRevalidate, 
		int maxAge, 
		int sharedMaxAge)
		: _mustRevalidate{mustRevalidate}
		, _noCache{noCache}
		, _noStore{noStore}
		, _noTransform{noTransform}
		, _accessControlPublic{accessControlPublic}
		, _accessControlPrivate{accessControlPrivate}
		, _proxyRevalidate{proxyRevalidate}
		, _maxAge{maxAge}
		, _sharedMaxAge{sharedMaxAge}
	{}

	/*static*/ std::optional<ResponseCacheControl> ResponseCacheControl::parseFromResponseHeaders(const std::map<std::string, std::string> &headers) 
	{
		std::map<std::string, std::string>::const_iterator cacheControlIter = headers.find("Cache-Control");
		if (cacheControlIter == headers.end()) {
			return std::nullopt;
		}

		const std::string& headerValue = cacheControlIter->second;
		std::map<std::string, std::string> parameterizedDirectives;
		std::set<std::string> directives;
		size_t last = 0;
		size_t next = 0;
		while ((next = headerValue.find(",", last)) != std::string::npos) {
			std::string directive = headerValue.substr(last, next - last);
			size_t equalSize = directive.find("=");
			if (equalSize != std::string::npos) {
				parameterizedDirectives.insert({ directive.substr(0, equalSize), directive.substr(equalSize + 1) });
			}
			else {
				directives.insert(directive);
			}

			last = next + 1;
		}
		directives.insert(headerValue.substr(last));

		bool mustRevalidate = directives.find("must-revalidate") != directives.end();
		bool noCache = directives.find("no-cache") != directives.end();
		bool noStore = directives.find("no-store") != directives.end();
		bool noTransform = directives.find("no-transform") != directives.end();
		bool accessControlPublic = directives.find("public") != directives.end();
		bool accessControlPrivate = directives.find("private") != directives.end();
		bool proxyRevalidate = directives.find("proxyRevalidate") != directives.end();

		int maxAge = 0;
		std::map<std::string, std::string>::const_iterator maxAgeIter = parameterizedDirectives.find("max-age");
		if (maxAgeIter != parameterizedDirectives.end()) {
			maxAge = std::stoi(maxAgeIter->second);
		}

		int sharedMaxAge = 0;
		std::map<std::string, std::string>::const_iterator sharedMaxAgeIter = parameterizedDirectives.find("s-maxage");
		if (sharedMaxAgeIter != parameterizedDirectives.end()) {
			sharedMaxAge = std::stoi(sharedMaxAgeIter->second);
		}

		return ResponseCacheControl(mustRevalidate, 
			noCache, 
			noStore, 
			noTransform, 
			accessControlPublic, 
			accessControlPrivate, 
			proxyRevalidate, 
			maxAge, 
			sharedMaxAge);
	}
}