#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include "MaterialWriter.h"
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialNormalTextureInfo.h>
#include <CesiumGltf/MaterialOcclusionTextureInfo.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Texture.h>
#include <magic_enum.hpp>
#include <vector>
#include <cassert>

void writePbrMetallicRoughness(const CesiumGltf::MaterialPBRMetallicRoughness& pbr, CesiumGltf::JsonWriter& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("pbrMetallicRoughness");
    j.StartObject();
    if (!pbr.baseColorFactor.empty()) {
        j.Key("baseColorFactor");
        assert(pbr.baseColorFactor.size() == 4);
        j.StartArray();
        j.Double(pbr.baseColorFactor[0]);
        j.Double(pbr.baseColorFactor[1]);
        j.Double(pbr.baseColorFactor[2]);
        j.Double(pbr.baseColorFactor[3]);
        j.EndArray();
    }

    if (pbr.baseColorTexture) {
        j.Key("baseColorTexture");
        j.StartObject();
        if (pbr.baseColorTexture->index >= 0) {
            j.Key("index");
            j.Int(pbr.baseColorTexture->index);
        }
        if (pbr.baseColorTexture->texCoord >= 0) {
            j.Key("texCoord");
            j.Int64(pbr.baseColorTexture->texCoord);
        }

        if (!pbr.baseColorTexture->extensions.empty()) {
            writeExtensions(pbr.baseColorTexture->extensions, j);
        }

        if (!pbr.baseColorTexture->extras.empty()) {
            j.Key("extras");
            CesiumGltf::writeJsonValue(pbr.baseColorTexture->extras, j);
        }

        j.EndObject();
    }
    
    if (!pbr.extensions.empty()) {
        writeExtensions(pbr.extensions, j);
    }

    if (!pbr.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonValue(pbr.extras, j, false);
    }

    j.EndObject();
}

void writeNormalTexture(const CesiumGltf::MaterialNormalTextureInfo& normalTexture, CesiumGltf::JsonWriter& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("normalTextureInfo");
    j.StartObject();

    j.Key("index");
    j.Int(normalTexture.index);

    if (normalTexture.texCoord > 0) {
        j.KeyPrimitive("texCoord", normalTexture.texCoord);
    }

    if (normalTexture.scale != 1) {
        j.KeyPrimitive("scale", normalTexture.scale);
    }

    if (!normalTexture.extensions.empty()) {
        writeExtensions(normalTexture.extensions, j);
    }
        
    if (!normalTexture.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonValue(normalTexture.extras, j, false);
    }

    j.EndObject();
}

void writeOcclusionTexture(const CesiumGltf::MaterialOcclusionTextureInfo& occlusionTexture, CesiumGltf::JsonWriter& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("occlusionTexture");
    j.StartObject();

    j.Key("index");
    j.Int(occlusionTexture.index);

    if (occlusionTexture.texCoord != 0) {
        j.Key("texCoord");
        j.Int64(occlusionTexture.texCoord);
    }

    if (occlusionTexture.strength != 1) {
        j.Key("strength");
        j.Double(occlusionTexture.strength);
    }

    if (!occlusionTexture.extensions.empty()) {
        writeExtensions(occlusionTexture.extensions, j);
    }

    if (!occlusionTexture.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonValue(occlusionTexture.extras, j, false);
    }

    j.EndObject();
}

void writeEmissiveTexture(const CesiumGltf::TextureInfo& emissiveTexture, CesiumGltf::JsonWriter& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("emissiveTexture");
    j.StartObject();
    j.Key("index");
    j.Int(emissiveTexture.index);
    if (emissiveTexture.texCoord != 0) {
        j.KeyPrimitive("texCoord", emissiveTexture.texCoord);
    }

    if (!emissiveTexture.extensions.empty()) {
        writeExtensions(emissiveTexture.extensions, j);
    }

    if (!emissiveTexture.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonValue(emissiveTexture.extras, j, false);
    }

    j.EndObject();
}

void CesiumGltf::writeMaterial(
    const std::vector<Material>& materials,
    CesiumGltf::JsonWriter& jsonWriter
) {
    if (materials.empty()) {
        return;
    }

    auto&j = jsonWriter;
    j.Key("materials");
    j.StartArray();

    for (const auto& material : materials) {
        j.StartObject();
        if (!material.name.empty()) {
            j.KeyPrimitive("name", material.name);
        }

        if (material.pbrMetallicRoughness) {
            writePbrMetallicRoughness(*material.pbrMetallicRoughness, jsonWriter);
        }

        if (material.normalTexture) {
            writeNormalTexture(*material.normalTexture, jsonWriter);
        }

        if (material.occlusionTexture) {
            writeOcclusionTexture(*material.occlusionTexture, jsonWriter);
        }

        if (material.emissiveTexture) {
            writeEmissiveTexture(*material.emissiveTexture, jsonWriter);
        }

        if (!material.emissiveFactor.empty()) {
            assert(material.emissiveFactor.size() == 3);
            j.Key("emissiveFactor");
            j.StartArray();
            j.Double(material.emissiveFactor[0]);
            j.Double(material.emissiveFactor[1]);
            j.Double(material.emissiveFactor[2]);
            j.EndArray();
        }

        if (material.alphaMode != Material::AlphaMode::OPAQUE) {
            j.KeyPrimitive("alphaMode", magic_enum::enum_name(material.alphaMode));
        }

        if (material.alphaCutoff != 0.5) {
            j.Key("alphaCutoff");
            j.Double(material.alphaCutoff);
        }

        if (material.doubleSided) {
            j.Key("doubleSided");
            j.Bool(material.doubleSided);
        }
        
        if (!material.extensions.empty()) {
            writeExtensions(material.extensions, j);
        }

        if (!material.extras.empty()) {
            j.Key("extras");
            writeJsonValue(material.extras, j, false);
        }

        j.EndObject();
    }
    j.EndArray();
}
