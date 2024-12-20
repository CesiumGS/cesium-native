
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/KhrTextureTransform.h>

#include <glm/trigonometric.hpp>

#include <optional>

using namespace CesiumGltf;

KhrTextureTransform::KhrTextureTransform() noexcept
    : _status(KhrTextureTransformStatus::Valid),
      _offset({0, 0}),
      _rotation(0),
      _scale({1.0, 1.0}),
      _texCoordSetIndex(std::nullopt),
      _rotationSineCosine({0, 1}) {}

KhrTextureTransform::KhrTextureTransform(
    const ExtensionKhrTextureTransform& extension) noexcept
    : _status(KhrTextureTransformStatus::Valid),
      _offset({0, 0}),
      _rotation(0),
      _scale({1.0, 1.0}),
      _texCoordSetIndex(std::nullopt),
      _rotationSineCosine({0, 1}) {

  if (extension.offset.size() != 2) {
    this->_status = KhrTextureTransformStatus::ErrorInvalidOffset;
    return;
  }
  this->_offset = {extension.offset[0], extension.offset[1]};

  this->_rotation = extension.rotation;
  this->_rotationSineCosine.x = glm::sin(this->_rotation);
  this->_rotationSineCosine.y = glm::cos(this->_rotation);

  if (extension.scale.size() != 2) {
    this->_status = KhrTextureTransformStatus::ErrorInvalidScale;
    return;
  }
  this->_scale = {extension.scale[0], extension.scale[1]};

  this->_texCoordSetIndex = extension.texCoord;
}

glm::dvec2
KhrTextureTransform::applyTransform(double u, double v) const noexcept {
  glm::dvec2 scaled{u * this->_scale.x, v * this->_scale.y};

  const double sin = this->_rotationSineCosine.x;
  const double cos = this->_rotationSineCosine.y;
  // u' = u cos(r) + v sin(r)
  // v' = v cos(r) - u sin(r)
  glm::dvec2 scaledAndRotated{0, 0};
  scaledAndRotated.x = scaled.x * cos + scaled.y * sin;
  scaledAndRotated.y = scaled.y * cos - scaled.x * sin;

  return scaledAndRotated + this->_offset;
}
