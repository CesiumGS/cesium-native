#include "CesiumGltf/PropertyTexturePropertyView.h"

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

double applySamplerWrapS(const double u, const int32_t wrapS) {
  if (wrapS == Sampler::WrapS::REPEAT) {
    double integral = 0;
    double fraction = std::modf(u, &integral);
    return fraction < 0 ? 1.0 - fraction : fraction;
  }

  if (wrapS == Sampler::WrapS::MIRRORED_REPEAT) {
    double integral = 0;
    double fraction = std::abs(std::modf(u, &integral));
    int64_t integer = static_cast<int64_t>(std::abs(integral));
    // If the integer part is odd, the direction is reversed.
    return integer % 2 == 1 ? 1.0 - fraction : fraction;
  }

  return glm::clamp(u, 0.0, 1.0);
}

double applySamplerWrapT(const double v, const int32_t wrapT) {
  if (wrapT == Sampler::WrapT::REPEAT) {
    double integral = 0;
    double fraction = std::modf(v, &integral);
    return fraction < 0 ? 1.0 - fraction : fraction;
  }

  if (wrapT == Sampler::WrapT::MIRRORED_REPEAT) {
    double integral = 0;
    double fraction = std::abs(std::modf(v, &integral));
    int64_t integer = static_cast<int64_t>(std::abs(integral));
    // If the integer part is odd, the direction is reversed.
    return integer % 2 == 1 ? 1.0 - fraction : fraction;
  }

  return glm::clamp(v, 0.0, 1.0);
}

} // namespace CesiumGltf
