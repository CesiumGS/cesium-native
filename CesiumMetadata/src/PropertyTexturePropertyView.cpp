#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumGltf/PropertyView.h>

namespace CesiumGltf {

// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidPropertyTexture;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidTexture;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidSampler;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidImage;
const PropertyViewStatusType PropertyTexturePropertyViewStatus::ErrorEmptyImage;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorInvalidChannels;
const PropertyViewStatusType
    PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch;

} // namespace CesiumGltf
