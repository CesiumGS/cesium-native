#include "Writer.h"
#include "AccessorWriter.h"
#include "AnimationWriter.h"
#include "AssetWriter.h"
#include "CesiumGltf/Writer.h"
#include "ImageWriter.h"
#include "JsonObjectWriter.h"
#include "JsonWriter.h"
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
#include <CesiumGltf/JsonValue.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <stdexcept>
#include <string_view>

void validateFlags(CesiumGltf::WriteFlags options) {
    if (options & CesiumGltf::WriteFlags::GLB &&
        options & CesiumGltf::WriteFlags::GLTF) {
        throw std::runtime_error("GLB and GLTF flags are mutually exclusive.");
    }
}

std::vector<std::uint8_t>
CesiumGltf::writeModelToByteArray(const Model& model, WriteFlags flags) {
    validateFlags(flags);
    std::vector<std::uint8_t> result;

    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    CesiumGltf::writeAccessor(model.accessors, writer);
    CesiumGltf::writeAnimation(model.animations, writer);
    CesiumGltf::writeAsset(model.asset, writer);
    auto buffers = CesiumGltf::writeBuffer(model.buffers, writer);
    CesiumGltf::writeBufferView(model.bufferViews, writer);
    CesiumGltf::writeCamera(model.cameras, writer);
    CesiumGltf::writeImage(model.images, writer);
    CesiumGltf::writeMaterial(model.materials, writer);
    CesiumGltf::writeMesh(model.meshes, writer);
    CesiumGltf::writeNode(model.nodes, writer);
    CesiumGltf::writeSampler(model.samplers, writer);
    CesiumGltf::writeScene(model.scenes, writer);
    CesiumGltf::writeSkin(model.skins, writer);
    CesiumGltf::writeTexture(model.textures, writer);
    CesiumGltf::writeExtensions(model.extensions, writer);

    // TODO: extensionsUsed
    // TODO: extensionsRequired

    if (!model.extras.empty()) {
        writeJsonObject(model.extras, writer);
    }

    writer.EndObject();
    std::string_view view(writer.toString());

    if (flags & WriteFlags::GLB) {
        return writeBinaryGLB(std::move(buffers), view);
    }

    return result;
}