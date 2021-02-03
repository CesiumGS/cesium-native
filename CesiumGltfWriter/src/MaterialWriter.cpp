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

void writePbrMetallicRoughness(const CesiumGltf::MaterialPBRMetallicRoughness& pbr, rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
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
        if (!pbr.baseColorTexture->extras.empty()) {
            j.Key("extras");
            CesiumGltf::writeJsonObject(pbr.baseColorTexture->extras, j);
        }
        // todo: extensions
        j.EndObject();
    }

    if (!pbr.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonObject(pbr.extras, j);
        // todo: extensions
    }
    // todo: extensions

    j.EndObject();
}

void writeNormalTexture(const CesiumGltf::MaterialNormalTextureInfo& normalTexture, rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("normalTextureInfo");
    j.StartObject();

    j.Key("index");
    j.Int(normalTexture.index);

    if (normalTexture.texCoord > 0) {
        j.Key("texCoord");
        j.Int64(normalTexture.texCoord);
    }

    if (normalTexture.scale != 1) {
        j.Key("scale");
        j.Double(normalTexture.scale);
    }

    if (!normalTexture.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonObject(normalTexture.extras, j);
    }

    // TODO: extensions
    j.EndObject();
}

void writeOcclusionTexture(const CesiumGltf::MaterialOcclusionTextureInfo& occlusionTexture, rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
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

    if (!occlusionTexture.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonObject(occlusionTexture.extras, j);
    }
    // todo extensions
    j.EndObject();
}

void writeEmissiveTexture(const CesiumGltf::TextureInfo& emissiveTexture, rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("emissiveTexture");
    j.StartObject();
    j.Key("index");
    j.Int(emissiveTexture.index);
    if (emissiveTexture.texCoord != 0) {
        j.Key("texCoord");
        j.Int64(emissiveTexture.texCoord);
    }

    if (!emissiveTexture.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonObject(emissiveTexture.extras, j);
    }

    // todo extensions 
    // todo additional properties
    j.EndObject();
}

void CesiumGltf::writeMaterial(
    const std::vector<Material>& materials,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
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
            j.Key("name");
            j.String(material.name.c_str());
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
            j.Key("alphaMode");
            auto alphaMode = std::string(magic_enum::enum_name(material.alphaMode));
            j.String(alphaMode.c_str());
        }

        if (material.alphaCutoff != 0.5) {
            j.Key("alphaCutoff");
            j.Double(material.alphaCutoff);
        }

        if (material.doubleSided) {
            j.Key("doubleSided");
            j.Bool(material.doubleSided);
        }

        if (!material.extras.empty()) {
            j.Key("extras");
            writeJsonObject(material.extras, j);
        }

        // TODO: additional properties are allowed
        j.EndObject();
    }
    j.EndArray();
}