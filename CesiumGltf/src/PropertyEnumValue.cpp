#include <CesiumGltf/PropertyEnumValue.h>

bool CesiumGltf::operator==(
    const PropertyEnumValue& lhs,
    const PropertyEnumValue& rhs) {
  return lhs.value() == rhs.value() && lhs.name() == rhs.name();
}