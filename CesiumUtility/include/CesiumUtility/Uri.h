#pragma once

#include <ada.h>
#include <ada/url_search_params.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace CesiumUtility {

/**
 * @brief A class for parsing and manipulating Uniform Resource Identifiers
 * (URIs).
 *
 * The URI parser supports the [WhatWG URL
 * specification](https://url.spec.whatwg.org/). It also supports
 * protocol-relative URIs such as `//example.com`, and opaque paths such as
 * `a/b/c`.
 */
class Uri final {
public:
  /**
   * @brief Attempts to create a new Uri by parsing the given string. If the
   * string fails to parse, \ref isValid will return false.
   *
   * @param uri A string containing the URI to attempt to parse.
   */
  Uri(const std::string& uri);
  /**
   * @brief Attempts to create a new Uri from a base URI and a relative URI. If
   * the base URI is invalid, only the relative URI string will be used. If the
   * relative URI fails to parse, \ref isValid will return false.
   *
   * @param base The base URI that the relative URI is relative to.
   * @param relative A string containing a relative URI to attempt to parse.
   * @param useBaseQuery If true, the resulting URI will include the query
   * parameters of both the base URI and the relative URI. If false, only the
   * relative URI's query parameters will be used (if any).
   */
  Uri(const Uri& base, const std::string& relative, bool useBaseQuery = false);

  /**
   * @brief Returns a string representation of the entire URI including path and
   * query parameters.
   */
  std::string_view toString() const;

  /**
   * @brief Returns true if this URI has been successfully parsed.
   */
  bool isValid() const;

  /**
   * @brief Equivalent to \ref isValid.
   */
  operator bool() const { return this->isValid(); }

  /**
   * @brief Gets the scheme portion of the URI. If the URI was created without a
   * scheme, this will return an empty string.
   *
   * @returns The scheme, or an empty string if the URI could not be parsed or
   * has no scheme.
   */
  std::string_view getScheme() const;

  /**
   * @brief Gets the host portion of the URI. If the URI also specifies a
   * non-default port, it will be included in the returned host. If the URI
   * contains no host, this will return an empty string.
   *
   * @returns The host, or an empty string if the URI could not be parsed or has
   * no host.
   */
  std::string_view getHost() const;

  /**
   * @brief Gets the path portion of the URI. This will not include query
   * parameters, if present.
   *
   * @return The path, or empty string if the URI could not be parsed.
   */
  std::string_view getPath() const;

  /**
   * @brief Gets the query portion of the URI.
   *
   * @return The path, or empty string if the URI could not be parsed.
   */
  std::string_view getQuery() const;

  /**
   * @brief Sets the path portion of a URI to a new value. The other portions of
   * the URI are left unmodified, including any query parameters.
   *
   * @param path The new path portion of the URI.
   */
  void setPath(const std::string_view& path);

  /**
   * @brief Sets the query portion of a URI to a new value. The other portions
   * of the URI are left unmodified.
   *
   * @param queryString The new query portion of the URI.
   */
  void setQuery(const std::string_view& queryString);

  /**
   * @brief Attempts to resolve a relative URI using a base URI.
   *
   * For example, a relative URI `/v1/example` together with the base URI
   * `https://api.cesium.com` would resolve to
   * `https://api.cesium.com/v1/example`.
   *
   * @param base The base URI that the relative URI is relative to.
   * @param relative The relative URI to be resolved against the base URI.
   * @param useBaseQuery If true, any query parameters of the base URI will be
   * retained in the resolved URI.
   * @param assumeHttpsDefault This parameter is ignored and is only kept for
   * API compatibility.
   * @returns The resolved URI.
   *
   * @deprecated Use the \ref Uri constructor instead.
   */
  static std::string resolve(
      const std::string& base,
      const std::string& relative,
      bool useBaseQuery = false,
      bool assumeHttpsDefault = true);
  /**
   * @brief Adds the given key and value to the query string of a URI. For
   * example, `addQuery("https://api.cesium.com/v1/example", "key", "value")`
   * would produce the URL `https://api.cesium.com/v1/example?key=value`.
   *
   * @param uri The URI whose query string will be modified.
   * @param key The key to be added to the query string.
   * @param value The value to be added to the query string.
   * @returns The modified URI including the new query string parameter.
   *
   * @deprecated Use the \ref UriQuery class:
   * ```
   * Uri parsedUri(uri);
   * UriQuery params(parsedUri);
   * params.setValue(key, value);
   * parsedUri.setQuery(params.toQueryString());
   * ```
   */
  static std::string addQuery(
      const std::string& uri,
      const std::string& key,
      const std::string& value);
  /**
   * @brief Obtains the value of the given key from the query string of the URI,
   * if possible.
   *
   * If the URI can't be parsed, or the key doesn't exist in the
   * query string, an empty string will be returned.
   *
   * @param uri The URI with a query string to obtain a value from.
   * @param key The key whose value will be obtained from the URI, if possible.
   * @returns The value of the given key in the query string, or an empty string
   * if not found.
   *
   * @deprecated Use the \ref UriQuery class:
   * ```
   * Uri parsedUri(uri);
   * UriQuery params(parsedUri);
   * params.getValue(key);
   * ```
   */
  static std::string
  getQueryValue(const std::string& uri, const std::string& key);

  /**
   * @brief A callback to fill-in a placeholder value in a URL.
   *
   * @param placeholder The text of the placeholder. For example, if the
   * placeholder was `{example}`, the value of `placeholder` will be `example`.
   * @returns The value to use in place of the placeholder.
   */
  typedef std::string
  SubstitutionCallbackSignature(const std::string& placeholder);

  /**
   * @brief Substitutes the placeholders in a templated URI with their
   * appropriate values obtained using a specified callback function.
   *
   * A templated URI has placeholders in the form of `{name}`. For example,
   * `https://example.com/{x}/{y}/{z}` has three placeholders, `x`, `y`, and `z`.
   * The callback will be called for each placeholder and they will be replaced
   * with the return value of this callback.
   *
   * @param templateUri The templated URI whose placeholders will be substituted
   * by this method.
   * @param substitutionCallback The callback that will be called for each
   * placeholder in the provided URI. See \ref SubstitutionCallbackSignature.
   */
  static std::string substituteTemplateParameters(
      const std::string& templateUri,
      const std::function<SubstitutionCallbackSignature>& substitutionCallback);

  /**
   * @brief Escapes a portion of a URI, percent-encoding disallowed characters.
   *
   * @param s The string to escape.
   * @return The escaped string.
   */
  static std::string escape(const std::string& s);

  /**
   * @brief Unescapes a portion of a URI, decoding any percent-encoded
   * characters.
   *
   * @param s The string to unescape.
   * @return The unescaped string.
   */
  static std::string unescape(const std::string& s);

  /**
   * @brief Converts a Unix file system path to a string suitable for use as the
   * path portion of a URI. Characters that are not allowed in the path portion
   * of a URI are percent-encoded as necessary.
   *
   * If the path is absolute (it starts with a slash), then the URI will start
   * with a slash as well.
   *
   * @param unixPath The file system path.
   * @return The URI path.
   */
  static std::string unixPathToUriPath(const std::string& unixPath);

  /**
   * @brief Converts a Windows file system path to a string suitable for use as
   * the path portion of a URI. Characters that are not allowed in the path
   * portion of a URI are percent-encoded as necessary.
   *
   * Either `/` or `\` may be used as a path separator on input, but the output
   * will contain only `/`.
   *
   * If the path is absolute (it starts with a slash or with C:\ or similar),
   * then the URI will start with a slash well.
   *
   * @param windowsPath The file system path.
   * @return The URI path.
   */
  static std::string windowsPathToUriPath(const std::string& windowsPath);

  /**
   * @brief Converts a file system path on the current system to a string
   * suitable for use as the path portion of a URI. Characters that are not
   * allowed in the path portion of a URI are percent-encoded as necessary.
   *
   * If the `_WIN32` preprocessor definition is defined when compiling
   * cesium-native, this is assumed to be a Windows-like system and this
   * function calls {@link windowsPathToUriPath}. Otherwise, this is assumed to
   * be a Unix-like system and this function calls {@link unixPathToUriPath}.
   *
   * @param nativePath The file system path.
   * @return The URI path.
   */
  static std::string nativePathToUriPath(const std::string& nativePath);

  /**
   * @brief Converts the path portion of a URI to a Unix file system path.
   * Percent-encoded characters in the URI are decoded.
   *
   * If the URI path is absolute (it starts with a slash), then the file system
   * path will start with a slash as well.
   *
   * @param uriPath The URI path.
   * @return The file system path.
   */
  static std::string uriPathToUnixPath(const std::string& uriPath);

  /**
   * @brief Converts the path portion of a URI to a Windows file system path.
   * Percent-encoded characters in the URI are decoded.
   *
   * If the URI path is absolute (it starts with a slash), then the file system
   * path will start with a slash or a drive letter.
   *
   * @param uriPath The URI path.
   * @return The file system path.
   */
  static std::string uriPathToWindowsPath(const std::string& uriPath);

  /**
   * @brief Converts the path portion of a URI to a file system path on the
   * current system. Percent-encoded characters in the URI are decoded.
   *
   * If the `_WIN32` preprocessor definition is defined when compiling
   * cesium-native, this is assumed to be a Windows-like system and this
   * function calls {@link uriPathToWindowsPath}. Otherwise, this is assumed to
   * be a Unix-like system and this function calls {@link uriPathToUnixPath}.
   *
   * @param uriPath The URI path.
   * @return The file system path.
   */
  static std::string uriPathToNativePath(const std::string& uriPath);

  /**
   * @brief Gets the path portion of the URI. This will not include path
   * parameters, if present.
   *
   * @param uri The URI from which to get the path.
   * @return The path, or empty string if the URI could not be parsed.
   *
   * @deprecated Create a \ref Uri instance and use \ref Uri::getPath() instead.
   */
  static std::string getPath(const std::string& uri);

  /**
   * @brief Sets the path portion of a URI to a new value. The other portions of
   * the URI are left unmodified, including any path parameters.
   *
   * @param uri The URI for which to set the path.
   * @param newPath The new path portion of the URI.
   * @returns The new URI after setting the path. If the original URI cannot be
   * parsed, it is returned unmodified.
   *
   * @deprecated Create a \ref Uri instance and use \ref Uri::setPath(const
   * std::string_view&) instead.
   */
  static std::string
  setPath(const std::string& uri, const std::string& newPath);

private:
  std::optional<ada::url_aggregator> _url = std::nullopt;
  bool _hasScheme = false;
};

/**
 * @brief A class for parsing and manipulating the query string of a URI.
 */
class UriQuery final {
public:
  /**
   * @brief Creates a \ref UriQuery object from a query string.
   *
   * This query string should be in the format
   * `key1=value1&key2=value2&key3=value3...`. This is the format returned by
   * \ref Uri::getQuery. This string can include percent-encoded values.
   *
   * @param queryString The query string to parse into a query params object.
   */
  UriQuery(const std::string_view& queryString) : _params(queryString) {}
  /**
   * @brief Creates a \ref UriQuery object from a \ref Uri instance.
   *
   * This is equivalent to `UriQuery(uri.getQuery())`.
   *
   * @param uri The URI instance to obtain the query params from.
   */
  UriQuery(const Uri& uri) : _params(uri.getQuery()) {}

  /**
   * @brief Obtains the value of the given key from the query parameters,
   * if possible.
   *
   * If the URI can't be parsed, or the key doesn't exist in the
   * query string, `std::nullopt` will be returned.
   *
   * @param key The key whose value will be obtained from the query string, if
   * possible.
   * @returns The value of the given key in the query string, or `std::nullopt`
   * if not found.
   */
  std::optional<std::string_view> getValue(const std::string& key) {
    return this->_params.get(key);
  }

  /**
   * @brief Sets the given key in the query parameters to the given value.
   * If the key doesn't exist already, it will be added to the query parameters.
   * Otherwise, the previous value will be overwritten.
   *
   * @param key The key to be added to the query string.
   * @param value The value to be added to the query string.
   */
  void setValue(const std::string& key, const std::string& value) {
    this->_params.set(key, value);
  }

  /**
   * @brief Returns true if this query string contains a value for the given
   * key, or false otherwise.
   *
   * @param key The key to check.
   */
  bool hasValue(const std::string& key) { return this->_params.has(key); }

  /**
   * @brief Converts this object back into a query string, including all
   * modifications that have been made. This result can be passed directly to
   * \ref Uri::setQuery.
   */
  std::string toQueryString() const { return this->_params.to_string(); }

  /** @brief Returns an iterator pointing to the beginning of the query
   * parameters. */
  inline auto begin() const { return this->_params.begin(); }
  /** @brief Returns an iterator pointing to the end of the query parameters. */
  inline auto end() const { return this->_params.end(); }
  /** @brief Returns the first element in the query parameters. */
  inline auto front() const { return this->_params.front(); }
  /** @brief Returns the last element in the query parameters. */
  inline auto back() const { return this->_params.back(); }

private:
  ada::url_search_params _params;
};

} // namespace CesiumUtility
