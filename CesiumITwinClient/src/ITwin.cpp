#include <CesiumITwinClient/ITwin.h>

#include <string>

namespace CesiumITwinClient {
ITwinStatus iTwinStatusFromString(const std::string& str) {
  if (str == "Active") {
    return ITwinStatus::Active;
  } else if (str == "Inactive") {
    return ITwinStatus::Inactive;
  } else if (str == "Trial") {
    return ITwinStatus::Trial;
  }

  return ITwinStatus::Unknown;
}
} // namespace CesiumITwinClient