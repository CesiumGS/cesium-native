#include "CesiumGltfWriter/Writer.h"

#include "AccessorWriter.h"
#include "AnimationWriter.h"
#include "AssetWriter.h"
#include "BufferViewWriter.h"
#include "BufferWriter.h"
#include "CameraWriter.h"
#include "ExtensionWriter.h"
#include "ImageWriter.h"
#include "MaterialWriter.h"
#include "MeshWriter.h"
#include "NodeWriter.h"
#include "SamplerWriter.h"
#include "SceneWriter.h"
#include "SkinWriter.h"
#include "TextureWriter.h"
#include "WriteBinaryGLB.h"

#include <CesiumGltfWriter/WriteGLTFCallback.h>
#include <CesiumGltfWriter/WriteModelOptions.h>
#include <CesiumGltfWriter/Writer.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/JsonValue.h>

#include <array>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace CesiumGltf;
using namespace CesiumUtility;

CesiumGltfWriter::WriteModelResult writeModel(
    const Model& model,
    const CesiumGltfWriter::WriteModelOptions& options,
    std::string_view filename,
    const CesiumGltfWriter::WriteGLTFCallback& writeGLTFCallback =
        CesiumGltfWriter::noopGltfWriter);

CesiumGltfWriter::WriteModelResult CesiumGltfWriter::writeModelAsEmbeddedBytes(
    const Model& model,
    const WriteModelOptions& options) {
  return writeModel(model, options, "");
}

CesiumGltfWriter::WriteModelResult CesiumGltfWriter::writeModelAndExternalFiles(
    const Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback) {
  return writeModel(model, options, filename, writeGLTFCallback);
}

CesiumGltfWriter::WriteModelResult writeModel(
    const Model& model,
    const CesiumGltfWriter::WriteModelOptions& options,
    std::string_view filename,
    const CesiumGltfWriter::WriteGLTFCallback& writeGLTFCallback) {

  CesiumGltfWriter::WriteModelResult result;
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

  CesiumGltfWriter::writeAccessor(model.accessors, *writer);
  CesiumGltfWriter::writeAnimation(result, model.animations, *writer);
  CesiumGltfWriter::writeAsset(model.asset, *writer);
  CesiumGltfWriter::writeBuffer(
      result,
      model.buffers,
      *writer,
      options,
      writeGLTFCallback);
  CesiumGltfWriter::writeBufferView(model.bufferViews, *writer);
  CesiumGltfWriter::writeCamera(model.cameras, *writer);
  CesiumGltfWriter::writeImage(
      result,
      model.images,
      *writer,
      options,
      writeGLTFCallback);
  CesiumGltfWriter::writeMaterial(model.materials, *writer);
  CesiumGltfWriter::writeMesh(model.meshes, *writer);
  CesiumGltfWriter::writeNode(model.nodes, *writer);
  CesiumGltfWriter::writeSampler(model.samplers, *writer);
  CesiumGltfWriter::writeScene(model.scenes, *writer);
  CesiumGltfWriter::writeSkin(model.skins, *writer);
  CesiumGltfWriter::writeTexture(model.textures, *writer);
  CesiumGltfWriter::writeExtensions(model.extensions, *writer);

  if (!model.extras.empty()) {
    CesiumJsonWriter::writeJsonValue(model.extras, *writer);
  }

  writer->EndObject();

  if (options.exportType == CesiumGltfWriter::GltfExportType::GLB) {
    if (model.buffers.empty()) {
      result.gltfAssetBytes = CesiumGltfWriter::writeBinaryGLB(
          std::vector<std::byte>{},
          writer->toStringView());
    }

    else {
      result.gltfAssetBytes = CesiumGltfWriter::writeBinaryGLB(
          model.buffers.at(0).cesium.data,
          writer->toStringView());
    }
  } else {
    result.gltfAssetBytes = writer->toBytes();
  }

  writeGLTFCallback(filename, result.gltfAssetBytes);
  return result;
}
