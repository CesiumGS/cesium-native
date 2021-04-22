#include "CesiumGltf/Writer.h"
#include "AccessorWriter.h"
#include "AnimationWriter.h"
#include "AssetWriter.h"
#include "ExtensionWriter.h"
#include "ImageWriter.h"
#include "JsonObjectWriter.h"
#include "JsonWriter.h"
#include "MaterialWriter.h"
#include "MeshWriter.h"
#include "NodeWriter.h"
#include "PrettyJsonWriter.h"
#include "SamplerWriter.h"
#include "SceneWriter.h"
#include "SkinWriter.h"
#include "TextureWriter.h"
#include "WriteBinaryGLB.h"
#include <BufferViewWriter.h>
#include <BufferWriter.h>
#include <CameraWriter.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/WriteModelOptions.h>
#include <CesiumGltf/Writer.h>
#include <CesiumUtility/JsonValue.h>
#include <array>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace CesiumGltf;
using namespace CesiumUtility;

WriteModelResult writeModel(
    const Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback = noopGltfWriter);

WriteModelResult CesiumGltf::writeModelAsEmbeddedBytes(
    const Model& model,
    const WriteModelOptions& options) {
  return writeModel(model, options, "");
}

WriteModelResult CesiumGltf::writeModelAndExternalFiles(
    const Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback) {
  return writeModel(model, options, filename, writeGLTFCallback);
}

WriteModelResult writeModel(
    const Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback) {

  WriteModelResult result;
  std::unique_ptr<JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<PrettyJsonWriter>();
  } else {
    writer = std::make_unique<JsonWriter>();
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

  CesiumGltf::writeAccessor(model.accessors, *writer);
  CesiumGltf::writeAnimation(result, model.animations, *writer);
  CesiumGltf::writeAsset(model.asset, *writer);
  CesiumGltf::writeBuffer(
      result,
      model.buffers,
      *writer,
      options,
      writeGLTFCallback);
  CesiumGltf::writeBufferView(model.bufferViews, *writer);
  CesiumGltf::writeCamera(model.cameras, *writer);
  CesiumGltf::writeImage(
      result,
      model.images,
      *writer,
      options,
      writeGLTFCallback);
  CesiumGltf::writeMaterial(model.materials, *writer);
  CesiumGltf::writeMesh(model.meshes, *writer);
  CesiumGltf::writeNode(model.nodes, *writer);
  CesiumGltf::writeSampler(model.samplers, *writer);
  CesiumGltf::writeScene(model.scenes, *writer);
  CesiumGltf::writeSkin(model.skins, *writer);
  CesiumGltf::writeTexture(model.textures, *writer);
  CesiumGltf::writeExtensions(model.extensions, *writer);

  if (!model.extras.empty()) {
    writeJsonValue(model.extras, *writer);
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
