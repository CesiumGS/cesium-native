#include "AssetWriter.h"
#include <CesiumGltf/Asset.h>

void CesiumGltf::writeAsset(
    const Asset& asset,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
) {
    auto& j = jsonWriter;
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

    // TODO: extensions, extras
}