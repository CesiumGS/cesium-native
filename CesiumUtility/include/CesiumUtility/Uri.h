#pragma once

#include <functional>
#include <string>

namespace CesiumUtility {
class Uri final {
public:
  static std::string resolve(
      const std::string& base,
      const std::string& relative,
      bool useBaseQuery = false,
      bool assumeHttpsDefault = true);
  static std::string addQuery(
      const std::string& uri,
      const std::string& key,
      const std::string& value);
  static std::string
  getQueryValue(const std::string& uri, const std::string& key);

  typedef std::string
  SubstitutionCallbackSignature(const std::string& placeholder);
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
   * @param The new path portion of the URI.
   * @returns The new URI after setting the path. If the original URI cannot be
   * parsed, it is returned unmodified.
   */
  static std::string
  setPath(const std::string& uri, const std::string& newPath);
};
} // namespace CesiumUtility
