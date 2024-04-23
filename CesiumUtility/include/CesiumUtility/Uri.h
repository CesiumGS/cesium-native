#pragma once

#include <functional>
#include <string>

namespace CesiumUtility {
class Uri final {
public:
  static std::string resolve(
      const std::string& base,
      const std::string& relative,
      bool useBaseQuery = false);
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

  static std::string escape(const std::string& s);

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
