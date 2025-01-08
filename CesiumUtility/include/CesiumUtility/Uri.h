#pragma once

#include <functional>
#include <string>

namespace CesiumUtility {
/**
 * @brief A class for building and manipulating Uniform Resource Identifiers
 * (URIs).
 */
class Uri final {
public:
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
   * @param assumeHttpsDefault If true, protocol-relative URIs (such as
   * `//api.cesium.com`) will be assumed to be `https`. If false, they will be
   * assumed to be `http`.
   * @returns The resolved URI.
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
   */
  static std::string
  setPath(const std::string& uri, const std::string& newPath);
};
} // namespace CesiumUtility
