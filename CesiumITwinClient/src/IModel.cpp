#include <CesiumITwinClient/IModel.h>

#include <string>

namespace CesiumITwinClient {
IModelState iModelStateFromString(const std::string& str) {
  if (str == "initialized") {
    return IModelState::Initialized;
  } else if (str == "notInitialized") {
    return IModelState::NotInitialized;
  }

  return IModelState::Unknown;
}
} // namespace CesiumITwinClient