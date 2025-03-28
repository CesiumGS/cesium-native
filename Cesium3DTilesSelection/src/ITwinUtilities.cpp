#include "ITwinUtilities.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <cstddef>
#include <span>
#include <string>

namespace Cesium3DTilesSelection {
void parseITwinErrorResponseIntoErrorList(
    const CesiumAsync::IAssetResponse& response,
    CesiumUtility::ErrorList& errors) {
  rapidjson::Document jsonResponse;
  const std::span<const std::byte> data = response.data();
  jsonResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (jsonResponse.HasParseError()) {
    errors.emplaceError(fmt::format(
        "Error when parsing iTwin API error message, error code {} at byte "
        "offset {}",
        jsonResponse.GetParseError(),
        jsonResponse.GetErrorOffset()));
    return;
  }

  const auto error = jsonResponse.FindMember("error");
  if (error == jsonResponse.MemberEnd() || !error->value.IsObject()) {
    // No additional error information available.
    return;
  }

  const rapidjson::Value::Object errorObj = error->value.GetObject();

  const std::string errorCode =
      CesiumUtility::JsonHelpers::getStringOrDefault(errorObj, "code", "");
  const std::string errorMessage =
      CesiumUtility::JsonHelpers::getStringOrDefault(errorObj, "message", "");

  std::string finalMessage =
      fmt::format("API error code '{}': {}", errorCode, errorMessage);

  const auto detailsIt = errorObj.FindMember("details");
  if (detailsIt != errorObj.MemberEnd() && detailsIt->value.IsArray()) {
    for (const rapidjson::Value& details : detailsIt->value.GetArray()) {
      const std::string detailsCode =
          CesiumUtility::JsonHelpers::getStringOrDefault(details, "code", "");
      const std::string detailsMessage =
          CesiumUtility::JsonHelpers::getStringOrDefault(
              details,
              "message",
              "");
      const std::string detailsTarget =
          CesiumUtility::JsonHelpers::getStringOrDefault(details, "target", "");
      if (!detailsTarget.empty()) {
        finalMessage += fmt::format(
            "\n\t- '{}' in '{}': {}",
            detailsCode,
            detailsTarget,
            detailsMessage);
      } else {
        finalMessage +=
            fmt::format("\n\t- '{}': {}", detailsCode, detailsMessage);
      }
    }
  }

  errors.emplaceError(finalMessage);
}

} // namespace Cesium3DTilesSelection