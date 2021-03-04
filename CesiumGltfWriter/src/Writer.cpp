#include "AccessorWriter.h"
#include "AnimationWriter.h"
#include "AssetWriter.h"
#include "CesiumGltf/Writer.h"
#include "ImageWriter.h"
#include "JsonObjectWriter.h"
#include "JsonWriter.h"
#include "PrettyJsonWriter.h"
#include "MaterialWriter.h"
#include "MeshWriter.h"
#include "NodeWriter.h"
#include "SamplerWriter.h"
#include "SceneWriter.h"
#include "SkinWriter.h"
#include "TextureWriter.h"
#include "ExtensionWriter.h"
#include "WriteBinaryGLB.h"
#include <BufferViewWriter.h>
#include <BufferWriter.h>
#include <CameraWriter.h>
#include <CesiumGltf/WriteFlags.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/Writer.h>
#include <CesiumGltf/JsonValue.h>
#include <stdexcept>
#include <string_view>
#include <optional>
#include <array>
#include <cstdio>

using namespace CesiumGltf;

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

std::vector<std::uint8_t> writeModel(
    const Model& model, 
    WriteFlags flags,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter
);

std::vector<std::uint8_t> CesiumGltf::writeModelAsEmbeddedBytes(
    const Model& model,
    WriteFlags flags
) {
    return writeModel(model, flags, "");
}

void CesiumGltf::writeModelAndExternalFiles(
    const Model& model,
    WriteFlags flags,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback
) {
    writeModel(model, flags, filename, writeGLTFCallback);
}

std::vector<std::uint8_t> writeModel(
    const Model& model, 
    WriteFlags flags,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback 
) {
    validateFlags(flags);
    std::vector<std::uint8_t> result;
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
    CesiumGltf::writeImage(model.images, *writer);
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
            result = writeBinaryGLB(std::vector<std::uint8_t>{}, writer->toStringView());
        }
        
        else {
            result = writeBinaryGLB(model.buffers.at(0).cesium.data, writer->toStringView());
        }
    } else {
        result = writer->toBytes();
    }

    writeGLTFCallback(filename, result);
    return result;
}
