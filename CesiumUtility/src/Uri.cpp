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

constexpr const char* HTTPS_PREFIX = "https:";
const std::string FILE_PREFIX = "file:///";

// C++ locale settings might change which values std::isalpha checks for. We only want ASCII.
bool isAsciiAlpha(char c) {
  return c >= 0x41 && c <= 0x7a && (c <= 0x5a || c >= 0x61);
}
bool isAscii(char c) { return c <= 0x7f; }

/**
 * A URI has a valid scheme if it starts with an ASCII alpha character and has a sequence of ASCII characters followed by a "://"
 */
bool urlHasScheme(const std::string& uri) {
  for (size_t i = 0; i < uri.length(); i++) {
    if (uri[i] == ':') {
      return uri.length() > i + 2 && uri [i + 1] == '/' && uri[i + 2] == '/';
    } else if (i == 0 && !isAsciiAlpha(uri[i])) {
      // Scheme must start with an ASCII alpha character
      return false;
    } else if (!isAscii(uri[i])) {
      // Scheme must be an ASCII string
      return false;
    }
  }

  return false;
}
} // namespace

Uri::Uri(const std::string& uri) {
  UrlResult result;
  if (uri.starts_with("//")) {
    // This is a protocol-relative URL.
    // We will treat it as an HTTPS URL.
    this->hasScheme = true;
    result = ada::parse(HTTPS_PREFIX + uri);
  } else {
    this->hasScheme = urlHasScheme(uri);
    result = this->hasScheme ? ada::parse(uri) : ada::parse(FILE_PREFIX + uri);
  }

  if (result) {
    this->url = std::make_unique<ada::url_aggregator>(std::move(result.value()));
    this->params =
        std::make_unique<ada::url_search_params>(this->url->get_search());
  }
}

Uri::Uri(const Uri& uri) {
  if (uri.url) {
    this->url = std::make_unique<ada::url_aggregator>(*uri.url);
    this->params =
        std::make_unique<ada::url_search_params>(this->url->get_search());
  }
  this->hasScheme = uri.hasScheme;
}

Uri::Uri(const Uri& base, const std::string& relative, bool useBaseQuery) {
  UrlResult result;
  if (!base.isValid()) {
    this->hasScheme = urlHasScheme(relative);
    result = this->hasScheme ? ada::parse(relative)
                             : ada::parse(FILE_PREFIX + relative);
  } else {
    this->hasScheme = base.hasScheme;
    result = ada::parse(relative, base.url.get());
  }

  if (result) {
    this->url =
        std::make_unique<ada::url_aggregator>(std::move(result.value()));
    this->params =
        std::make_unique<ada::url_search_params>(this->url->get_search());

    if (useBaseQuery) {
      // Set from relative to base to give priority to relative URL query string
      for (const auto& [key, value] : *base.params) {
        if (!this->params->has(key)) {
          this->params->set(key, value);
        }
      }
      this->url->set_search(this->params->to_string());
    }
  }
}

std::string Uri::toString() const {
  if (!this->url) {
    return "";
  }

  // Update URL with any param modifications
  this->url->set_search(this->params->to_string());
  const std::string_view result = this->url->get_href();
  return this->hasScheme ? std::string(result)
                         : std::string(result.substr(FILE_PREFIX.length()));
}

bool Uri::isValid() const {
  return this->url != nullptr && this->params != nullptr; }

const std::optional<std::string_view>
Uri::getQueryValue(const std::string& key) const {
  if (!this->isValid()) {
    return std::nullopt;
  }

  return this->params->get(key);
}

void Uri::setQueryValue(const std::string& key, const std::string& value) {
  if (!this->isValid()) {
    return;
  }

  this->params->set(key, value);
}

const std::string_view Uri::getPath() const {
  if (!this->isValid()) {
    return {};
  }

  return this->url->get_pathname();
}

void Uri::setPath(const std::string_view& path) {
  this->url->set_pathname(path);
}

std::string Uri::resolve(
    const std::string& base,
    const std::string& relative,
    bool useBaseQuery,
    [[maybe_unused]] bool assumeHttpsDefault) {
  return Uri(Uri(base), relative, useBaseQuery).toString();
}

std::string Uri::addQuery(
    const std::string& uri,
    const std::string& key,
    const std::string& value) {
  Uri parsedUri(uri);
  if (!parsedUri.isValid()) {
    return uri;
  }

  parsedUri.setQueryValue(key, value);
  return parsedUri.toString();
}

std::string Uri::getQueryValue(const std::string& uri, const std::string& key) {
  return std::string(Uri(uri).getQueryValue(key).value_or(""));
}

std::string Uri::substituteTemplateParameters(
    const std::string& templateUri,
    const std::function<SubstitutionCallbackSignature>& substitutionCallback) {
  const std::string& decodedUri =
      ada::unicode::percent_decode(templateUri, templateUri.find('%'));
  std::string result;
  std::string placeholder;

  size_t startPos = 0;
  size_t nextPos;

  // Find the start of a parameter
  while ((nextPos = decodedUri.find('{', startPos)) != std::string::npos) {
    result.append(decodedUri, startPos, nextPos - startPos);

    // Find the end of this parameter
    ++nextPos;
    const size_t endPos = decodedUri.find('}', nextPos);
    if (endPos == std::string::npos) {
      throw std::runtime_error("Unclosed template parameter");
    }

    placeholder = decodedUri.substr(nextPos, endPos - nextPos);
    result.append(substitutionCallback(placeholder));

    startPos = endPos + 1;
  }

  result.append(decodedUri, startPos, templateUri.length() - startPos);

  return result;
}

std::string Uri::escape(const std::string& s) { return ada::unicode::percent_encode(s, ada::character_sets::WWW_FORM_URLENCODED_PERCENT_ENCODE); }

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

std::string Uri::getPath(const std::string& uri) { return std::string(Uri(uri).getPath()); }

std::string Uri::setPath(const std::string& uri, const std::string& newPath) {
  Uri parsedUri(uri);
  parsedUri.setPath(newPath);
  return parsedUri.toString();
}

} // namespace CesiumUtility
