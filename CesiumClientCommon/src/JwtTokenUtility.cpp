#include <CesiumClientCommon/JwtTokenUtility.h>

#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

using namespace CesiumUtility;

namespace CesiumClientCommon {
CesiumUtility::Result<rapidjson::Document>
JwtTokenUtility::parseTokenPayload(const std::string_view& tokenStr) {
  const size_t firstPeriod = tokenStr.find('.');
  if (firstPeriod == std::string::npos) {
    return Result<rapidjson::Document>(ErrorList::error(
        "Invalid JWT token, format must be `header.payload.signature`."));
  }

  const size_t secondPeriod = tokenStr.find('.', firstPeriod + 1);
  if (secondPeriod == std::string::npos) {
    return Result<rapidjson::Document>(ErrorList::error(
        "Invalid JWT token, format must be `header.payload.signature`."));
  }

  std::string payloadSegment(
      tokenStr.substr(firstPeriod + 1, secondPeriod - firstPeriod - 1));
  size_t payloadLength = payloadSegment.length();
  // modp_b64 requires padded base64 strings, which JWT tokens are not.
  if (payloadLength % 4 != 0) {
    const size_t newSize = (payloadLength / 4 + 1) * 4;
    payloadSegment.resize(newSize);
    for (size_t i = payloadLength; i != newSize; i++) {
      payloadSegment[i] = '=';
    }
    payloadLength = newSize;
  }

  const size_t b64Len = modp_b64_decode_len(payloadSegment.length());
  std::string decodedPayload;
  decodedPayload.resize(b64Len, '\0');

  if (modp_b64_decode(
          decodedPayload.data(),
          payloadSegment.data(),
          payloadLength) == size_t(-1)) {
    return Result<rapidjson::Document>(
        ErrorList::error("Unable to decode base64 payload."));
  }

  rapidjson::Document json;
  json.Parse(decodedPayload.data(), decodedPayload.size());

  if (json.HasParseError()) {
    return Result<rapidjson::Document>(ErrorList::error(fmt::format(
        "Failed to parse payload JSON, parse error {} at byte offset {}.",
        rapidjson::GetParseError_En(json.GetParseError()),
        json.GetErrorOffset())));
  }

  if (!json.IsObject()) {
    return Result<rapidjson::Document>(
        ErrorList::error("Missing payload contents."));
  }

  return Result<rapidjson::Document>(std::move(json));
}
} // namespace CesiumClientCommon