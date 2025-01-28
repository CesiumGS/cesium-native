#include <CesiumUtility/Uri.h>

#include <ada/character_sets-inl.h>
#include <ada/implementation.h>
#include <ada/unicode.h>
#include <ada/url_aggregator.h>
#include <ada/url_search_params.h>

#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

namespace CesiumUtility {

namespace {
const std::string HTTPS_PREFIX = "https:";
const std::string FILE_PREFIX = "file:///";
const char WINDOWS_PATH_SEP = '\\';
const char PATH_SEP = '/';

const std::regex TEMPLATE_REGEX(
    "\\{(.*?)\\}", std::regex::optimize);

using UrlResult = ada::result<ada::url_aggregator>;

// C++ locale settings might change which values std::isalpha checks for. We
// only want ASCII.
bool isAsciiAlpha(unsigned char c) {
  return c >= 0x41 && c <= 0x7a && (c <= 0x5a || c >= 0x61);
}
bool isAscii(unsigned char c) { return c <= 0x7f; }

/**
 * A URI has a valid scheme if it starts with an ASCII alpha character and has a
 * sequence of ASCII characters followed by a "://"
 */
bool urlHasScheme(const std::string& uri) {
  for (size_t i = 0; i < uri.length(); i++) {
    unsigned char c = static_cast<unsigned char>(uri[i]);
    if (c == ':') {
      return uri.length() > i + 2 && uri[i + 1] == '/' && uri[i + 2] == '/';
    } else if ((i == 0 && !isAsciiAlpha(c)) || !isAscii(c)) {
      // Scheme must start with an ASCII alpha character and be an ASCII string
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
    this->_hasScheme = true;
    result = ada::parse(HTTPS_PREFIX + uri);
  } else {
    this->_hasScheme = urlHasScheme(uri);
    result = this->_hasScheme ? ada::parse(uri) : ada::parse(FILE_PREFIX + uri);
  }

  if (result) {
    this->_url.emplace(std::move(result.value()));
    this->_params.emplace(this->_url->get_search());
  }
}

Uri::Uri(const Uri& uri) {
  if (uri._url) {
    this->_url.emplace(*uri._url);
    this->_params.emplace(this->_url->get_search());
  }
  this->_hasScheme = uri._hasScheme;
}

Uri::Uri(const Uri& base, const std::string& relative, bool useBaseQuery) {
  UrlResult result;
  if (!base.isValid()) {
    this->_hasScheme = urlHasScheme(relative);
    result = this->_hasScheme ? ada::parse(relative)
                             : ada::parse(FILE_PREFIX + relative);
  } else {
    this->_hasScheme = base._hasScheme;
    result = ada::parse(relative, &base._url.value());
  }

  if (result) {
    this->_url.emplace(std::move(result.value()));
    this->_params.emplace(this->_url->get_search());

    if (useBaseQuery) {
      // Set from relative to base to give priority to relative URL query string
      for (const auto& [key, value] : *base._params) {
        if (!this->_params->has(key)) {
          this->_params->set(key, value);
        }
      }
      this->_url->set_search(this->_params->to_string());
    }
  }
}

std::string Uri::toString() const {
  if (!this->_url) {
    return "";
  }

  const std::string_view result = this->_url->get_href();
  return this->_hasScheme ? std::string(result)
                         : std::string(result.substr(FILE_PREFIX.length()));
}

bool Uri::isValid() const {
  return this->_url && this->_params;
}

const std::optional<std::string_view>
Uri::getQueryValue(const std::string& key) {
  if (!this->isValid()) {
    return std::nullopt;
  }

  return this->_params->get(key);
}

void Uri::setQueryValue(const std::string& key, const std::string& value) {
  if (!this->isValid()) {
    return;
  }

  this->_params->set(key, value);
  // Update URL with modified params
  this->_url->set_search(this->_params->to_string());
}

std::string_view Uri::getPath() const {
  if (!this->isValid()) {
    return {};
  }

  // Remove leading '/'
  return this->_url->get_pathname();
}

void Uri::setPath(const std::string_view& path) {
  this->_url->set_pathname(path);
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

  std::string result;
  // The output string will *probably* be at least as long as the input string.
  result.reserve(templateUri.length());
  std::string placeholder;

  std::sregex_iterator begin(
      templateUri.begin(),
      templateUri.end(),
      TEMPLATE_REGEX),
      end;

  size_t startPos = 0;

  for (auto i = begin; i != end; i++) {
    if (i->ready()) {
      const size_t position = static_cast<size_t>(i->position(0));
      if (position > startPos) {
        result.append(templateUri.substr(startPos, position - startPos));
      }

      placeholder = templateUri.substr(
          static_cast<size_t>(i->position(2)),
          static_cast<size_t>(i->length(2)));
      result += ada::unicode::percent_encode(
          substitutionCallback(placeholder),
          ada::character_sets::WWW_FORM_URLENCODED_PERCENT_ENCODE);

      startPos = position + static_cast<size_t>(i->length(0));
    }
  }

  // Append rest of URL if any remaining
  if (startPos < templateUri.length() - 1) {
    result.append(
        templateUri.substr(startPos, templateUri.length() - startPos));
  }

  return result;
}

std::string Uri::escape(const std::string& s) {
  return ada::unicode::percent_encode(
      s,
      ada::character_sets::WWW_FORM_URLENCODED_PERCENT_ENCODE);
}

std::string Uri::unescape(const std::string& s) {
  return ada::unicode::percent_decode(s, s.find('%'));
}

std::string Uri::unixPathToUriPath(const std::string& unixPath) {
  return Uri::nativePathToUriPath(unixPath);
}

std::string Uri::windowsPathToUriPath(const std::string& windowsPath) {
  return Uri::nativePathToUriPath(windowsPath);
}

std::string Uri::nativePathToUriPath(const std::string& nativePath) {
  const std::string encoded = ada::unicode::percent_encode(
      nativePath,
      ada::character_sets::PATH_PERCENT_ENCODE);

  const bool startsWithDriveLetter =
      encoded.length() >= 2 &&
      isAsciiAlpha(static_cast<unsigned char>(encoded[0])) && encoded[1] == ':';

  std::string output;
  output.reserve(encoded.length() + (startsWithDriveLetter ? 1 : 0));

  // Paths like C:/... should be prefixed with a path separator
  if (startsWithDriveLetter) {
    output += PATH_SEP;
  }

  // All we really need to do from here is convert our slashes
  for (size_t i = 0; i < encoded.length(); i++) {
    if (encoded[i] == WINDOWS_PATH_SEP) {
      output += PATH_SEP;
    } else {
      output += encoded[i];
    }
  }

  return output;
}

std::string Uri::uriPathToUnixPath(const std::string& uriPath) {
  // URI paths are pretty much just unix paths with URL encoding
  const std::string_view& rawPath = uriPath;
  return ada::unicode::percent_decode(rawPath, rawPath.find('%'));
}

std::string Uri::uriPathToWindowsPath(const std::string& uriPath) {
  const std::string path =
      ada::unicode::percent_decode(uriPath, uriPath.find('%'));

  size_t i = 0;
  // A path including a drive name will start like /C:/....
  // In that case, we just skip the first slash and continue on
  if (path.length() >= 3 && path[0] == '/' &&
      isAsciiAlpha(static_cast<unsigned char>(path[1])) && path[2] == ':') {
    i++;
  }

  std::string output;
  output.reserve(path.length() - i);
  for (; i < path.length(); i++) {
    if (path[i] == PATH_SEP) {
      output += WINDOWS_PATH_SEP;
    } else {
      output += path[i];
    }
  }

  return output;
}

std::string Uri::uriPathToNativePath(const std::string& uriPath) {
#ifdef _WIN32
  return uriPathToWindowsPath(uriPath);
#else
  return uriPathToUnixPath(uriPath);
#endif
}

std::string Uri::getPath(const std::string& uri) {
  return std::string(Uri(uri).getPath());
}

std::string Uri::setPath(const std::string& uri, const std::string& newPath) {
  Uri parsedUri(uri);
  parsedUri.setPath(newPath);
  return parsedUri.toString();
}

} // namespace CesiumUtility
