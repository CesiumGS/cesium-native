#include "Uri.h"
#include "uriparser/Uri.h"
#include <stdexcept>

std::string Uri::resolve(const std::string& base, const std::string& relative, bool useBaseQuery)
{
	UriUriA baseUri;

	if (uriParseSingleUriA(&baseUri, base.c_str(), nullptr) != URI_SUCCESS)
	{
		// Could not parse the base, so just use the relative directly and hope for the best.
		return relative;
	}

	UriUriA relativeUri;
	if (uriParseSingleUriA(&relativeUri, relative.c_str(), nullptr) != URI_SUCCESS)
	{
		// Could not parse one of the URLs, so just use the relative directly and hope for the best.
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	UriUriA resolvedUri;
	if (uriAddBaseUriA(&resolvedUri, &relativeUri, &baseUri) != URI_SUCCESS) {
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	if (uriNormalizeSyntaxA(&resolvedUri) != URI_SUCCESS)
	{
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	int charsRequired;
	if (uriToStringCharsRequiredA(&resolvedUri, &charsRequired) != URI_SUCCESS) {
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	std::string result(charsRequired, ' ');

	if (uriToStringA(const_cast<char*>(result.c_str()), &resolvedUri, charsRequired + 1, nullptr) != URI_SUCCESS) {
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	if (useBaseQuery)
	{
		std::string query(baseUri.query.first, baseUri.query.afterLast);
		if (resolvedUri.query.first)
		{
			result += "&" + query;
		}
		else
		{
			result += "?" + query;
		}
	}

	uriFreeUriMembersA(&resolvedUri);
	uriFreeUriMembersA(&relativeUri);
	uriFreeUriMembersA(&baseUri);

	return result;
}

std::string Uri::addQuery(const std::string& uri, const std::string& key, const std::string& value)
{
	// TODO
	if (uri.find('?') != std::string::npos) {
		return uri + "&" + key + "=" + value;
	} else {
		return uri + "?" + key + "=" + value;
	}
	//UriUriA baseUri;

	//if (uriParseSingleUriA(&baseUri, uri.c_str(), nullptr) != URI_SUCCESS)
	//{
	//	// TODO: report error
	//	return uri;
	//}

	//uriFreeUriMembersA(&baseUri);
}

std::string Uri::substituteTemplateParameters(const std::string& templateUri, const std::function<SubstitutionCallbackSignature>& substitutionCallback) {
	std::string result;
	std::string placeholder;

	size_t startPos = 0;
	size_t nextPos;

	// Find the start of a parameter
	while ((nextPos = templateUri.find('{', startPos)) != std::string::npos) {
		result.append(templateUri, startPos, nextPos - startPos);

		// Find the end of this parameter
		++nextPos;
		size_t endPos = templateUri.find('}', nextPos);
		if (endPos == std::string::npos) {
			throw std::runtime_error("Unclosed template parameter");
		}

		placeholder = templateUri.substr(nextPos, endPos - nextPos);
		result.append(substitutionCallback(placeholder));

		startPos = endPos + 1;
	}

	result.append(templateUri, startPos, templateUri.length() - startPos);

	return result;
}