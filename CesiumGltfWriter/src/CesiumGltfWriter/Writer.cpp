#include "CesiumGltfWriter/Writer.h"

#include "CesiumGltfWriter/AccessorWriter.h"
#include "CesiumGltfWriter/AnimationWriter.h"
#include "CesiumGltfWriter/AssetWriter.h"
#include "CesiumGltfWriter/BufferViewWriter.h"
#include "CesiumGltfWriter/BufferWriter.h"
#include "CesiumGltfWriter/CameraWriter.h"
#include "CesiumGltfWriter/ExtensionWriter.h"
#include "CesiumGltfWriter/ImageWriter.h"
#include "CesiumGltfWriter/MaterialWriter.h"
#include "CesiumGltfWriter/MeshWriter.h"
#include "CesiumGltfWriter/NodeWriter.h"
#include "CesiumGltfWriter/SamplerWriter.h"
#include "CesiumGltfWriter/SceneWriter.h"
#include "CesiumGltfWriter/SkinWriter.h"
#include "CesiumGltfWriter/TextureWriter.h"
#include "CesiumGltfWriter/WriteBinaryGLB.h"
#include "CesiumGltfWriter/WriteGLTFCallback.h"
#include "CesiumGltfWriter/WriteModelOptions.h"
#include "CesiumGltfWriter/WriteModelResult.h"

#include <CesiumGltf/Model.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/JsonValue.h>

#include <array>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace CesiumGltfWriter {
WriteModelResult writeModel(
    const CesiumGltf::Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback = noopGltfWriter);

WriteModelResult writeModelAsEmbeddedBytes(
    const CesiumGltf::Model& model,
    const WriteModelOptions& options) {
  return writeModel(model, options, "");
}

WriteModelResult writeModelAndExternalFiles(
    const CesiumGltf::Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback) {
  return writeModel(model, options, filename, writeGLTFCallback);
}

WriteModelResult writeModel(
    const CesiumGltf::Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback) {

  WriteModelResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  writer->StartObject();

  if (!model.extensionsUsed.empty()) {
    writer->KeyArray("extensionsUsed", [&]() {
      for (const auto& extensionUsed : model.extensionsUsed) {
        writer->String(extensionUsed);
      }
    });
  }

  if (!model.extensionsRequired.empty()) {
    writer->KeyArray("extensionsRequired", [&]() {
      for (const auto& extensionRequired : model.extensionsRequired) {
        writer->String(extensionRequired);
      }
    });
  }

  writeAccessor(model.accessors, *writer);
  writeAnimation(result, model.animations, *writer);
  writeAsset(model.asset, *writer);
  writeBuffer(result, model.buffers, *writer, options, writeGLTFCallback);
  writeBufferView(model.bufferViews, *writer);
  writeCamera(model.cameras, *writer);
  writeImage(result, model.images, *writer, options, writeGLTFCallback);
  writeMaterial(model.materials, *writer);
  writeMesh(model.meshes, *writer);
  writeNode(model.nodes, *writer);
  writeSampler(model.samplers, *writer);
  writeScene(model.scenes, *writer);
  writeSkin(model.skins, *writer);
  writeTexture(model.textures, *writer);
  writeExtensions(model.extensions, *writer);

  if (!model.extras.empty()) {
    CesiumJsonWriter::writeJsonValue(model.extras, *writer);
  }

  writer->EndObject();

  if (options.exportType == GltfExportType::GLB) {
    if (model.buffers.empty()) {
      result.gltfAssetBytes =
          writeBinaryGLB(std::vector<std::byte>{}, writer->toStringView());
    }

    else {
      result.gltfAssetBytes = writeBinaryGLB(
          model.buffers.at(0).cesium.data,
          writer->toStringView());
    }
  } else {
    result.gltfAssetBytes = writer->toBytes();
  }

  writeGLTFCallback(filename, result.gltfAssetBytes);
  return result;
}

} // namespace CesiumGltfWriter