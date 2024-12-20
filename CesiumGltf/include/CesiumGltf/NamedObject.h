#pragma once

#include <CesiumGltf/Library.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <string>

namespace CesiumGltf {
/**
 * @brief The base class for objects in a glTF that have a name.
 *
 * A named object is also an {@link CesiumUtility::ExtensibleObject}.
 */
struct CESIUMGLTF_API NamedObject : public CesiumUtility::ExtensibleObject {
  /**
   * @brief The user-defined name of this object.
   *
   * This is not necessarily unique, e.g., an accessor and a buffer could have
   * the same name, or two accessors could even have the same name.
   */
  std::string name;

  /**
   * @brief Calculates the size in bytes of this object, including the contents
   * of all collections, pointers, and strings. Calling this method may be slow
   * as it requires traversing the object's entire structure.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += sizeof(NamedObject);
    accum +=
        ExtensibleObject::getSizeBytes() - int64_t(sizeof(ExtensibleObject));
    accum += this->name.capacity() * sizeof(char);
    return accum;
  }
};
} // namespace CesiumGltf
