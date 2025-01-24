#pragma once

#include <CesiumGltf/ExtensionKhrTextureTransform.h>

#include <glm/glm.hpp>

namespace CesiumGltf {
/**
 * @brief The status of a parsed KHR_texture_transform.
 *
 * The {@link KhrTextureTransform} constructor always completes successfully,
 * but it may not always reflect the actual transform if errors occur. This
 * enumeration provides the reason.
 */
enum class KhrTextureTransformStatus {
  Valid,
  ErrorInvalidOffset,
  ErrorInvalidScale
};

/**
 * @brief A utility class that parses KHR_texture_transform parameters and
 * transforms input texture coordinates.
 */
class KhrTextureTransform {
public:
  /**
   * @brief Constructs a texture transformation with identity values.
   */
  KhrTextureTransform() noexcept;

  /**
   * @brief Constructs a texture transformation from the given extension.
   */
  KhrTextureTransform(const ExtensionKhrTextureTransform& extension) noexcept;

  /**
   * @brief The current \ref KhrTextureTransformStatus of the transform
   * operation.
   */
  KhrTextureTransformStatus status() const noexcept { return this->_status; }

  /**
   * @brief Gets the offset of the texture transform.
   */
  glm::dvec2 offset() const noexcept { return this->_offset; }

  /**
   * @brief Gets the rotation (in radians) of the texture transform.
   */
  double rotation() const noexcept { return this->_rotation; }

  /**
   * @brief Gets the sine and cosine of the rotation in the texture transform.
   * This is cached to avoid re-computing the values in the future.
   */
  glm::dvec2 rotationSineCosine() const noexcept {
    return this->_rotationSineCosine;
  }

  /**
   * @brief Gets the scale of the texture transform.
   */
  glm::dvec2 scale() const noexcept { return this->_scale; }

  /**
   * @brief Applies this texture transformation to the input coordinates.
   */
  glm::dvec2 applyTransform(double u, double v) const noexcept;

  /**
   * @brief Gets the texture coordinate set index used by this texture
   * transform. If defined, this should override the set index of the texture's
   * original textureInfo.
   */
  std::optional<int64_t> getTexCoordSetIndex() const noexcept {
    return this->_texCoordSetIndex;
  }

private:
  KhrTextureTransformStatus _status;
  glm::dvec2 _offset;
  double _rotation;
  glm::dvec2 _scale;
  std::optional<int64_t> _texCoordSetIndex;

  // Cached values of sin(rotation) and cos(rotation).
  glm::dvec2 _rotationSineCosine;
};

} // namespace CesiumGltf
