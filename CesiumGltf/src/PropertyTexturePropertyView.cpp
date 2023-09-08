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

std::array<uint8_t, 4> sampleNearestPixel(
    const ImageCesium& image,
    const std::vector<int64_t>& channels,
    const double u,
    const double v) {
  // For nearest filtering, std::floor is used instead of std::round.
  // This is because filtering is supposed to consider the pixel centers. But
  // memory access here acts as sampling the beginning of the pixel. Example:
  // 0.4 * 2 = 0.8. In a 2x1 pixel image, that should be closer to the left
  // pixel's center. But it will round to 1.0 which corresponds to the right
  // pixel. So the right pixel has a bigger range than the left one, which is
  // incorrect.
  double xCoord = std::floor(u * image.width);
  double yCoord = std::floor(v * image.height);

  // Clamp to ensure no out-of-bounds data access
  int64_t x = glm::clamp(
      static_cast<int64_t>(xCoord),
      static_cast<int64_t>(0),
      static_cast<int64_t>(image.width) - 1);
  int64_t y = glm::clamp(
      static_cast<int64_t>(yCoord),
      static_cast<int64_t>(0),
      static_cast<int64_t>(image.height) - 1);

  int64_t pixelIndex =
      image.bytesPerChannel * image.channels * (y * image.width + x);

  // TODO: Currently stb only outputs uint8 pixel types. If that
  // changes this should account for additional pixel byte sizes.
  const uint8_t* pValue =
      reinterpret_cast<const uint8_t*>(image.pixelData.data() + pixelIndex);

  std::array<uint8_t, 4> channelValues{0, 0, 0, 0};
  size_t len = glm::min(channels.size(), channelValues.size());
  for (size_t i = 0; i < len; i++) {
    channelValues[i] = *(pValue + channels[i]);
  }

  return channelValues;
}

} // namespace CesiumGltf
