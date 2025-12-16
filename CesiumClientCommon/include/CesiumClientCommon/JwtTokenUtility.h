#include <CesiumClientCommon/Library.h>
#include <CesiumUtility/Result.h>

#include <rapidjson/document.h>

namespace CesiumClientCommon {
/**
 * Utilities for handling JWT authentication tokens, like those used for Cesium
 * ion and Bentley iTwin.
 */
class CESIUMCLIENTCOMMON_API JwtTokenUtility {
public:
  /**
   * Parses the payload of the provided JWT token, returning a
   * `CesiumUtility::Result` containing either the `rapidjson::Document` of the
   * payload contents or an error.
   *
   * @param tokenStr The JWT token string to parse.
   */
  static CesiumUtility::Result<rapidjson::Document>
  parseTokenPayload(const std::string_view& tokenStr);
};
} // namespace CesiumClientCommon