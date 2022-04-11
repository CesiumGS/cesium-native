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

  typedef std::string
  SubstitutionCallbackSignature(const std::string& placeholder);
  static std::string substituteTemplateParameters(
      const std::string& templateUri,
      const std::function<SubstitutionCallbackSignature>& substitutionCallback);

  static std::string escape(const std::string& s);
};
} // namespace CesiumUtility
