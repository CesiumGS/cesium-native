#include "AssetWriter.h"
#include "JsonObjectWriter.h"
#include <CesiumGltf/Asset.h>

void CesiumGltf::writeAsset(
    const Asset& asset,
    CesiumGltf::JsonWriter& jsonWriter
) {
    auto& j = jsonWriter;
    j.Key("asset");
    j.StartObject();

    if (asset.copyright) {
        j.Key("copyright");
        j.String(asset.copyright->c_str());
    }

    if (asset.generator) {
        j.Key("generator");
        j.String(asset.generator->c_str());
    }

    j.Key("version");
    j.String(asset.version.c_str());

    if (asset.minVersion) {
        j.Key("minVersion");
        j.String(asset.minVersion->c_str());
    }

    if (!asset.extras.empty()) {
        j.Key("extras");
        writeJsonObject(asset.extras, j);
    }

    j.EndObject();

    // TODO: extensions
}