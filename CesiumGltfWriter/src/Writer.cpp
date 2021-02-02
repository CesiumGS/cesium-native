#include "CesiumGltf/Writer.h"
#include "AccessorWriter.h"
#include "AnimationWriter.h"
#include "AssetWriter.h"
#include "ImageWriter.h"
#include "NodeWriter.h"
#include "SceneWriter.h"
#include <BufferViewWriter.h>
#include <BufferWriter.h>
#include <CameraWriter.h>
#include <CesiumGltf/JsonValue.h>
#include <MaterialWriter.h>
#include <SamplerWriter.h>
#include <iostream>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>

std::vector<std::byte>
CesiumGltf::writeModelToByteArray(const Model& model, WriteOptions options) {

    std::vector<std::byte> result;
    if (options != (WriteOptions::GLB | WriteOptions::EmbedBuffers |
                    WriteOptions::EmbedImages)) {
        throw std::runtime_error(
            "Unsupported configuration. Only (GLB + "
            "EmbedBuffers + EmbedImages) supported for now.");
    }

    rapidjson::StringBuffer strBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuffer);
    writer.StartObject();
    CesiumGltf::writeAccessor(model.accessors, writer);
    CesiumGltf::writeAnimation(model.animations, writer);
    CesiumGltf::writeAsset(model.asset, writer);
    auto buffers = CesiumGltf::writeBuffer(model.buffers, writer);
    CesiumGltf::writeBufferView(model.bufferViews, writer);
    CesiumGltf::writeCamera(model.cameras, writer);
    CesiumGltf::writeImage(model.images, writer);
    CesiumGltf::writeMaterial(model.materials, writer);
    CesiumGltf::writeNode(model.nodes, writer);
    CesiumGltf::writeSampler(model.samplers, writer);
    CesiumGltf::writeScene(model.scenes, writer);
    // Skin
    // Texture
    // TextureInfo

    // TODO: remove this after debugging
    writer.EndObject();
    std::cout << strBuffer.GetString() << std::endl;
    return result;
}