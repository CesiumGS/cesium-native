#pragma once

#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/TextureInfo.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <vector>

namespace CesiumGltf {

struct Model;

/**
 * @brief Describes options for constructing a view on a glTF texture.
 */
struct TextureViewOptions {
  /**
   * @brief Whether to automatically apply the `KHR_texture_transform` extension
   * to the texture view, if it exists.
   *
   * A glTF texture may contain the `KHR_texture_transform` extension, which
   * transforms the texture coordinates used to sample the texture. The
   * extension may also override the TEXCOORD set index that was specified by
   * the original texture info.
   *
   * If a view is constructed with applyKhrTextureTransformExtension set to
   * true, it should automatically apply the texture transform to any UV
   * coordinates used to sample the texture. If the extension defines its own
   * TEXCOORD set index, it will override the original value.
   *
   * Otherwise, if the flag is set to false, UVs will not be transformed and
   * the original TEXCOORD set index will be preserved. The extension's values
   * may still be retrieved using getTextureTransform, if desired.
   */
  bool applyKhrTextureTransformExtension = false;

  /**
   * @brief Whether to copy the input image.
   *
   * By default, a view is constructed on the input glTF image without copying
   * its pixels. This can be problematic for clients that move or delete the
   * original glTF model. When this flag is true, the view will manage its own
   * copy of the pixel data to avoid such issues.
   */
  bool makeImageCopy = false;
};

/**
 * @brief Indicates the status of a texture view.
 *
 * The {@link TextureView} constructor always completes
 * successfully. However it may not always reflect the actual content of the
 * corresponding texture. This enumeration provides the reason.
 */
enum class TextureViewStatus {
  /**
   * @brief This texture view is valid and ready to use.
   */
  Valid,

  /**
   * @brief This texture view has not yet been initialized.
   */
  ErrorUninitialized,

  /**
   * @brief This texture view does not have a valid texture index.
   */
  ErrorInvalidTexture,

  /**
   * @brief This texture view does not have a valid sampler index.
   */
  ErrorInvalidSampler,

  /**
   * @brief This texture view does not have a valid image index.
   */
  ErrorInvalidImage,

  /**
   * @brief This texture is viewing an empty image.
   */
  ErrorEmptyImage,

  /**
   * @brief The image for this texture has channels that take up more than a
   * byte. Only single-byte channels are supported.
   */
  ErrorInvalidBytesPerChannel,
};

/**
 * @brief A view into the texture data of a single texture from a \ref Model.
 */
class TextureView {
public:
  /**
   * @brief Constructs an empty, uninitialized texture view.
   */
  TextureView() noexcept;

  /**
   * @brief Constructs a view of the texture specified by the given {@link TextureInfo}.
   *
   * @param model The glTF model in which to look for the texture's data.
   * @param textureInfo The texture info to create a view for.
   * @param options The options for constructing the view.
   */
  TextureView(
      const Model& model,
      const TextureInfo& textureInfo,
      const TextureViewOptions& options = TextureViewOptions()) noexcept;

  /**
   * @brief Constructs a view of the texture specified by the given {@link Sampler}
   * and {@link ImageAsset}.
   *
   * @param sampler The {@link Sampler} used by the texture.
   * @param image The {@link ImageAsset} used by the texture.
   * @param textureCoordinateSetIndex The set index for the `TEXCOORD_n`
   * attribute used to sample this texture.
   * @param pKhrTextureTransformExtension A pointer to the KHR_texture_transform
   * extension on the texture, if it exists.
   * @param options The options for constructing the view.
   */
  TextureView(
      const Sampler& sampler,
      const ImageAsset& image,
      int64_t textureCoordinateSetIndex,
      const ExtensionKhrTextureTransform* pKhrTextureTransformExtension =
          nullptr,
      const TextureViewOptions& options = TextureViewOptions()) noexcept;

  /**
   * @brief Get the status of this texture view.
   *
   * If invalid, it will not be safe to sample from this view.
   */
  TextureViewStatus getTextureViewStatus() const noexcept {
    return this->_textureViewStatus;
  }

  /**
   * @brief Get the texture coordinate set index for this view.
   * If this view was constructed with options.applyKhrTextureTransformExtension
   * as true, and if the texture contains the `KHR_texture_transform` extension,
   * then this will return the value from the extension since it is meant to
   * override the original index. However, if the extension does not specify a
   * TEXCOORD set index, then the original index of the texture is returned.
   */
  int64_t getTexCoordSetIndex() const noexcept {
    if (this->_applyTextureTransform && this->_textureTransform) {
      return this->_textureTransform->getTexCoordSetIndex().value_or(
          this->_texCoordSetIndex);
    }
    return this->_texCoordSetIndex;
  }

  /**
   * @brief Get the sampler describing how to sample the data from the
   * property's texture.
   *
   * This will be nullptr if the property texture property view runs into
   * problems during construction.
   */
  const Sampler* getSampler() const noexcept { return this->_pSampler; }

  /**
   * @brief Get the image containing this property's data. If this view was
   * constructed with options.makeImageCopy set to true, this will return a
   * pointer to the copied image.
   *
   * This will be nullptr if the texture view runs into
   * problems during construction.
   */
  const ImageAsset* getImage() const noexcept {
    if (this->_pImageCopy) {
      return this->_pImageCopy.get();
    }
    return this->_pImage.get();
  }

  /**
   * @brief Get the KHR_texture_transform for this texture if it exists.
   *
   * Even if this view was constructed with
   * options.applyKhrTextureTransformExtension set to false, it will save the
   * extension's values, and they may be retrieved through this function.
   *
   * If this view was constructed with applyKhrTextureTransformExtension set
   * to true, any texture coordinates passed into `get` or `getRaw` will be
   * automatically transformed, so there's no need to re-apply the transform
   * here.
   */
  std::optional<KhrTextureTransform> getTextureTransform() const noexcept {
    return this->_textureTransform;
  }

  /**
   * @brief Samples the image at the specified texture coordinates using NEAREST
   * pixel filtering, returning the bytes as uint8_t values. A channels vector
   * must be supplied to specify how many image channels are needed, and in what
   * order the bytes should be retrieved.
   */
  std::vector<uint8_t> sampleNearestPixel(
      double u,
      double v,
      const std::vector<int64_t>& channels) const noexcept;

private:
  TextureViewStatus _textureViewStatus;

  const Sampler* _pSampler;
  CesiumUtility::IntrusivePointer<ImageAsset> _pImage;
  int64_t _texCoordSetIndex;

  bool _applyTextureTransform;
  std::optional<KhrTextureTransform> _textureTransform;

  CesiumUtility::IntrusivePointer<ImageAsset> _pImageCopy;
};
} // namespace CesiumGltf
