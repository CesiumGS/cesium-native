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
#include <CesiumGltf/WriteFlags.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/Writer.h>
#include <CesiumUtility/JsonValue.h>
#include <array>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace CesiumGltf;
using namespace CesiumUtility;

void validateFlags(WriteFlags options) {
  const auto isGLB = options & WriteFlags::GLB;
  const auto isGLTF = options & WriteFlags::GLTF;

  if (isGLB && isGLTF) {
    throw std::runtime_error("GLB and GLTF flags are mutually exclusive.");
  }

  if (!isGLB && !isGLTF) {
    throw std::runtime_error("GLB or GLTF must be specified.");
  }
}

std::vector<std::byte> writeModel(
    const Model& model,
    WriteFlags flags,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);

std::vector<std::byte>
CesiumGltf::writeModelAsEmbeddedBytes(const Model& model, WriteFlags flags) {
  return writeModel(model, flags, "");
}

void CesiumGltf::writeModelAndExternalFiles(
    const Model& model,
    WriteFlags flags,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback) {
  writeModel(model, flags, filename, writeGLTFCallback);
}

std::vector<std::byte> writeModel(
    const Model& model,
    WriteFlags flags,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback) {
  validateFlags(flags);
  std::vector<std::byte> result;
  std::unique_ptr<JsonWriter> writer;

  if (flags & WriteFlags::PrettyPrint) {
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
  CesiumGltf::writeAnimation(model.animations, *writer);
  CesiumGltf::writeAsset(model.asset, *writer);
  CesiumGltf::writeBuffer(model.buffers, *writer, flags, writeGLTFCallback);
  CesiumGltf::writeBufferView(model.bufferViews, *writer);
  CesiumGltf::writeCamera(model.cameras, *writer);
  CesiumGltf::writeImage(model.images, *writer, flags, writeGLTFCallback);
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

  if (flags & WriteFlags::GLB) {
    if (model.buffers.empty()) {
      result = writeBinaryGLB(std::vector<std::byte>{}, writer->toStringView());
    }

    else {
      result = writeBinaryGLB(
          model.buffers.at(0).cesium.data,
          writer->toStringView());
    }
  } else {
    result = writer->toBytes();
  }

  writeGLTFCallback(filename, result);
  return result;
}
