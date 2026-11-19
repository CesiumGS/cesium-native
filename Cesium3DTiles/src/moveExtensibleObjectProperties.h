#include <CesiumUtility/ExtensibleObject.h>

#include <utility>

using namespace CesiumUtility;

namespace Cesium3DTiles {
inline void moveExtensibleObjectProperties(
    CesiumUtility::ExtensibleObject& destination,
    CesiumUtility::ExtensibleObject& source) {
  // Extensions are omitted since they are not necessarily transferrable between
  // 3D Tiles 1.1 and 2.0.
  destination.extras = std::move(source.extras);
  destination.unknownProperties = std::move(source.unknownProperties);
}

} // namespace Cesium3DTiles
