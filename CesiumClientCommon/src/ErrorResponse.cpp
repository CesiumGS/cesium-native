#include <CesiumClientCommon/ErrorResponse.h>

#include <CesiumUtility/JsonHelpers.h>

#include <fmt/format.h>
#include <rapidjson/document.h>

namespace CesiumClientCommon {
bool parseErrorResponse(
    const std::span<const std::byte>& body,
    std::string& outError,
    std::string& outErrorDesc) {
  rapidjson::Document doc;
  doc.Parse(reinterpret_cast<const char*>(body.data()), body.size());

  if (doc.HasParseError() || !doc.IsObject()) {
    return false;
  }

  outError = CesiumUtility::JsonHelpers::getStringOrDefault(doc, "error", "");
  outErrorDesc = CesiumUtility::JsonHelpers::getStringOrDefault(
      doc,
      "error_description",
      "");
  
  const auto& detailsMember = doc.FindMember("details");
  if(detailsMember != doc.MemberEnd() && detailsMember->value.IsArray()) {
    for(const auto& value : detailsMember->value.GetArray()) {
      const std::string code = CesiumUtility::JsonHelpers::getStringOrDefault(value, "code", "");
      const std::string message = CesiumUtility::JsonHelpers::getStringOrDefault(value, "message", "");
      const std::string target = CesiumUtility::JsonHelpers::getStringOrDefault(value, "target", "");
      if(target.empty()) {
        outErrorDesc += fmt::format("\n\t - {}: {}", code, message);
      } else {
        outErrorDesc += fmt::format("\n\t - {} in {}: {}", code, target, message);
      }
    }
  }

  return true;
}
} // namespace CesiumClientCommon