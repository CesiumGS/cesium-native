#include <CesiumITwinClient/ITwinRealityData.h>

#include <string>

namespace CesiumITwinClient {
ITwinRealityDataClassification
iTwinRealityDataClassificationFromString(const std::string& str) {
  if (str == "Terrain") {
    return ITwinRealityDataClassification::Terrain;
  } else if (str == "Imagery") {
    return ITwinRealityDataClassification::Imagery;
  } else if (str == "Pinned") {
    return ITwinRealityDataClassification::Pinned;
  } else if (str == "Model") {
    return ITwinRealityDataClassification::Model;
  } else if (str == "Undefined") {
    return ITwinRealityDataClassification::Undefined;
  }

  return ITwinRealityDataClassification::Unknown;
}
} // namespace CesiumITwinClient