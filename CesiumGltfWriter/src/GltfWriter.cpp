#include "CesiumGltf/GltfWriter.h"

namespace {

ModelWriterResult writeJsonModel(
    const CesiumJsonWriter::ExtensionWriterContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("CesiumGltf::ModelWriter::writeJsonModel");

  ModelJsonHandler modelHandler(context);
  WriteJsonResult<Model> jsonResult = JsonWriter::writeJson(data, modelHandler);

  return ModelWriterResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

void postprocess(
    const GltfWriter& writer,
    ModelWriterResult& writeModel,
    const WriteModelOptions& options) {
  Model& model = writeModel.model.value();

  if (options.decodeDataUrls) {
    decodeDataUrls(writer, writeModel, options.clearDecodedDataUrls);
  }

  if (options.decodeEmbeddedImages) {
    CESIUM_TRACE("CesiumGltf::decodeEmbeddedImages");
    for (Image& image : model.images) {
      const BufferView& bufferView =
          Model::getSafe(model.bufferViews, image.bufferView);
      const Buffer& buffer = Model::getSafe(model.buffers, bufferView.buffer);

      if (bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
        writeModel.warnings.emplace_back(
            "Image bufferView's byte offset is " +
            std::to_string(bufferView.byteOffset) + " and the byteLength is " +
            std::to_string(bufferView.byteLength) + ", the result is " +
            std::to_string(bufferView.byteOffset + bufferView.byteLength) +
            ", which is more than the available " +
            std::to_string(buffer.cesium.data.size()) + " bytes.");
        continue;
      }

      const gsl::span<const std::byte> bufferSpan(buffer.cesium.data);
      const gsl::span<const std::byte> bufferViewSpan = bufferSpan.subspan(
          static_cast<size_t>(bufferView.byteOffset),
          static_cast<size_t>(bufferView.byteLength));
      ImageWriterResult imageResult = writer.writeImage(bufferViewSpan);
      writeModel.warnings.insert(
          writeModel.warnings.end(),
          imageResult.warnings.begin(),
          imageResult.warnings.end());
      writeModel.errors.insert(
          writeModel.errors.end(),
          imageResult.errors.begin(),
          imageResult.errors.end());
      if (imageResult.image) {
        image.cesium = std::move(imageResult.image.value());
      } else {
        if (image.mimeType) {
          writeModel.errors.emplace_back(
              "Declared image MIME Type: " + image.mimeType.value());
        } else {
          writeModel.errors.emplace_back("Image does not declare a MIME Type");
        }
      }
    }
  }

  if (options.decodeDraco) {
    decodeDraco(writeModel);
  }
}

} // namespace

GltfWriter::GltfWriter() : _context() {
  this->_context.registerExtension<
      MeshPrimitive,
      KHR_draco_mesh_compressionJsonHandler>();

  this->_context
      .registerExtension<Model, ModelEXT_feature_metadataJsonHandler>();
  this->_context.registerExtension<
      MeshPrimitive,
      MeshPrimitiveEXT_feature_metadataJsonHandler>();
}

CesiumJsonWriter::ExtensionWriterContext& GltfWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
GltfWriter::getExtensions() const {
  return this->_context;
}

ModelWriterResult GltfWriter::writeModelAsEmbeddedBytes(
    const Model& model,
    const WriteModelOptions& options) const {

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();
  ModelWriterResult result = isBinaryGltf(data)
                                 ? writeBinaryModel(context, data)
                                 : writeJsonModel(context, data);

  if (result.model) {
    postprocess(*this, result, options);
  }

  return result;
}
