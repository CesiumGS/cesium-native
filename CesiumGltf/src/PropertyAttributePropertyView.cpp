#include <CesiumGltf/PropertyAttributePropertyView.h>
#include <CesiumGltf/PropertyView.h>

namespace CesiumGltf {
// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorInvalidPropertyAttribute;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorMissingAttribute;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorInvalidAccessor;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorAccessorTypeMismatch;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorAccessorComponentTypeMismatch;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorAccessorNormalizationMismatch;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorInvalidBufferView;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorInvalidBuffer;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorAccessorOutOfBounds;
const PropertyViewStatusType
    PropertyAttributePropertyViewStatus::ErrorBufferViewOutOfBounds;
} // namespace CesiumGltf
