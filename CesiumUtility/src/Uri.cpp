#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <ada.h>
#include <ada/character_sets-inl.h>
#include <ada/encoding_type.h>
#include <ada/unicode.h>
#include <ada/url_aggregator.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace CesiumUtility {

using UrlResult = ada::result<ada::url_aggregator>;

namespace {
void conformWindowsPathInPlace(std::string& path) {
  // Treated as a Unix path, c:/file.txt will be /c:/file.txt.
  if (path.length() >= 3 && path[0] == '/' && std::isalpha(path[1]) &&
      path[2] == ':') {
    path.erase(0, 1);
  }

  // Replace slashes in-place
  for (size_t i = 0; i < path.length(); i++) {
    if (path[i] == '/') {
      path[i] = '\\';
    }
  }
}

std::string urlEncode(const std::string_view& value) {
  std::ostringstream output;
  output << std::hex;
  output.fill('0');

  for (size_t i = 0; i < value.length(); i++) {
    // RFC 3986 unreserved characters
    if (std::isalnum(value[i]) || value[i] == '-' || value[i] == '.' ||
        value[i] == '_' || value[i] == '~') {
      output << value[i];
    } else {
      output << std::uppercase;
      output << '%';
      output << std::setw(2) << value[i];
      output << std::nouppercase;
    }
  }

  return output.str();
}

const char* const HTTPS_PREFIX = "https:";
const char* const HTTP_PREFIX = "http:";

UrlResult parseUrlConform(const std::string& url, bool assumeHttpsDefault) {
  if (url.starts_with("//")) {
    return ada::parse(
        std::string(assumeHttpsDefault ? HTTPS_PREFIX : HTTP_PREFIX) + url);
  }
  return ada::parse(url);
}

UrlResult parseUrlConform(const std::string& url, const ada::url_aggregator* base, bool assumeHttpsDefault) {
  if (url.starts_with("//")) {
    return ada::parse(
        std::string(assumeHttpsDefault ? HTTPS_PREFIX : HTTP_PREFIX) + url, base);
  }
  return ada::parse(url, base);
}
} // namespace

std::string Uri::resolve(
    const std::string& base,
    const std::string& relative,
    bool useBaseQuery,
    [[maybe_unused]] bool assumeHttpsDefault) {
  UrlResult baseUrl = parseUrlConform(base, assumeHttpsDefault);
  if (!baseUrl) {
    return relative;
  }

  UrlResult relativeUrl =
      parseUrlConform(relative, &baseUrl.value(), assumeHttpsDefault);
  if (!relativeUrl) {
    return relative;
  }

  if (useBaseQuery) {
    relativeUrl->set_search(baseUrl->get_search());
  }

  return std::string(relativeUrl->get_href());
}

std::string Uri::addQuery(
    const std::string& uri,
    const std::string& key,
    const std::string& value) {
  // TODO
  if (uri.find('?') != std::string::npos) {
    return uri + "&" + key + "=" + value;
  }
  return uri + "?" + key + "=" + value;
}

std::string Uri::getQueryValue(const std::string& url, const std::string& key) {
  UrlResult parsedUrl = parseUrlConform(url, true);
  if (!parsedUrl) {
    return "";
  }

  ada::url_search_params params(parsedUrl->get_search());
  return std::string(params.get(key).value_or(""));
}

std::string Uri::substituteTemplateParameters(
    const std::string& templateUri,
    const std::function<SubstitutionCallbackSignature>& substitutionCallback) {
  std::string result;
  std::string placeholder;

  size_t startPos = 0;
  size_t nextPos;

  // Find the start of a parameter
  while ((nextPos = templateUri.find('{', startPos)) != std::string::npos) {
    result.append(templateUri, startPos, nextPos - startPos);

    // Find the end of this parameter
    ++nextPos;
    const size_t endPos = templateUri.find('}', nextPos);
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

std::string Uri::escape(const std::string& s) { return urlEncode(s); }

std::string Uri::unescape(const std::string& s) {
  return ada::unicode::percent_decode(s, s.find('%'));
}

std::string Uri::unixPathToUriPath(const std::string& unixPath) {
  return nativePathToUriPath(unixPath);
}

std::string Uri::windowsPathToUriPath(const std::string& windowsPath) {
  return nativePathToUriPath(windowsPath);
}

std::string Uri::nativePathToUriPath(const std::string& nativePath) {
  UrlResult parsedUrl = ada::parse("file://" + nativePath);
  if (!parsedUrl) {
    return nativePath;
  }

  std::string result(parsedUrl->get_pathname());
  return result;
}

std::string Uri::uriPathToUnixPath(const std::string& uriPath) {
  ada::url_aggregator url;
  url.set_pathname(uriPath);
  std::string result = ada::unicode::percent_decode(
      url.get_pathname(),
      url.get_pathname().find('%'));
  if (result.starts_with("/") && !uriPath.starts_with("/")) {
    result.erase(0, 1);
  }
  return result;
}

std::string Uri::uriPathToWindowsPath(const std::string& uriPath) {
  std::string result = uriPathToUnixPath(uriPath);
  conformWindowsPathInPlace(result);
  return result;
}

std::string Uri::uriPathToNativePath(const std::string& uriPath) {
#ifdef _WIN32
  return uriPathToWindowsPath(uriPath);
#else
  return uriPathToUnixPath(uriPath);
#endif
}

std::string Uri::getPath(const std::string& uri) {
  UrlResult result = parseUrlConform(uri, true);
  if (!result) {
    return "";
  }

  std::string path(result->get_pathname());
  return path;
}

std::string Uri::setPath(const std::string& uri, const std::string& newPath) {
  UrlResult result = parseUrlConform(uri, true);
  if (!result) {
    return "";
  }

  result->set_pathname(newPath);
  return std::string(result->get_href());
}

} // namespace CesiumUtility
