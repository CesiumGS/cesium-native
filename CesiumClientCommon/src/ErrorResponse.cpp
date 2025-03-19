#include <CesiumClientCommon/ErrorResponse.h>
#include <CesiumUtility/JsonHelpers.h>

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <cstddef>
#include <span>
#include <string>

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

  rapidjson::Value::Object rootObj = doc.GetObject();

  const auto errorMember = doc.FindMember("error");
  if (errorMember != doc.MemberEnd() && errorMember->value.IsObject()) {
    rootObj = errorMember->value.GetObject();
    outError =
        CesiumUtility::JsonHelpers::getStringOrDefault(rootObj, "code", "");
    outErrorDesc =
        CesiumUtility::JsonHelpers::getStringOrDefault(rootObj, "message", "");
  } else {
    outError =
        CesiumUtility::JsonHelpers::getStringOrDefault(rootObj, "error", "");
    outErrorDesc = CesiumUtility::JsonHelpers::getStringOrDefault(
        rootObj,
        "error_description",
        "");
  }

  if (outError.empty() && outErrorDesc.empty()) {
    return false;
  }

  const auto& detailsMember = rootObj.FindMember("details");
  if (detailsMember != rootObj.MemberEnd() && detailsMember->value.IsArray()) {
    for (const auto& value : detailsMember->value.GetArray()) {
      const std::string code =
          CesiumUtility::JsonHelpers::getStringOrDefault(value, "code", "");
      const std::string message =
          CesiumUtility::JsonHelpers::getStringOrDefault(value, "message", "");
      const std::string target =
          CesiumUtility::JsonHelpers::getStringOrDefault(value, "target", "");
      if (target.empty()) {
        outErrorDesc += fmt::format("\n\t - {}: {}", code, message);
      } else {
        outErrorDesc +=
            fmt::format("\n\t - {} in {}: {}", code, target, message);
      }
    }
  }

  return true;
}
} // namespace CesiumClientCommon
