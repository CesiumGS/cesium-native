#pragma once

#include <string>
#include <functional>

class Uri final
{
public:
	static std::string resolve(const std::string& base, const std::string& relative, bool useBaseQuery = false);
	static std::string addQuery(const std::string& uri, const std::string& key, const std::string& value);

	typedef std::string SubstitutionCallbackSignature(const std::string& placeholder);
	static std::string substituteTemplateParameters(const std::string& templateUri, const std::function<SubstitutionCallbackSignature>& substitutionCallback);
};