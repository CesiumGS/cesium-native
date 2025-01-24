#include <CesiumGltf/PropertyView.h>

using namespace CesiumGltf;

// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType PropertyViewStatus::Valid;
const PropertyViewStatusType PropertyViewStatus::EmptyPropertyWithDefault;
const PropertyViewStatusType PropertyViewStatus::ErrorNonexistentProperty;
const PropertyViewStatusType PropertyViewStatus::ErrorTypeMismatch;
const PropertyViewStatusType PropertyViewStatus::ErrorComponentTypeMismatch;
const PropertyViewStatusType PropertyViewStatus::ErrorArrayTypeMismatch;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidNormalization;
const PropertyViewStatusType PropertyViewStatus::ErrorNormalizationMismatch;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidOffset;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidScale;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidMax;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidMin;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidNoDataValue;
const PropertyViewStatusType PropertyViewStatus::ErrorInvalidDefaultValue;
