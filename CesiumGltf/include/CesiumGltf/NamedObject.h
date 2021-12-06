#pragma once

#include "CesiumGltf/Library.h"

#include <CesiumUtility/ExtensibleObject.h>

#include <string>

namespace CesiumGltf {
/**
 * @brief The base class for objects in a glTF that have a name.
 *
 * A named object is also an {@link ExtensibleObject}.
 */
struct CESIUMGLTF_API NamedObject : public CesiumUtility::ExtensibleObject {
  /**
   * @brief The user-defined name of this object.
   *
   * This is not necessarily unique, e.g., an accessor and a buffer could have
   * the same name, or two accessors could even have the same name.
   */
  std::string name;
};
} // namespace CesiumGltf
