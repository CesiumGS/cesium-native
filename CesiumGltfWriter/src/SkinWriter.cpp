#include "SkinWriter.h"
void CesiumGltf::writeSkin(
    const std::vector<Skin>& skins,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {

    if (skins.empty()) {
        return;
    }

    auto& j = jsonWriter;

    j.Key("skins");
    j.StartArray();
    for (const auto& skin : skins) {
        j.StartObject();
        if (skin.inverseBindMatrices >= 0) {
             j.Key("inverseBindMatrices");
             j.Int(skin.inverseBindMatrices);
        }

        if (skin.skeleton >= 0) {
            j.Key("skeleton");
            j.Int(skin.skeleton);
        }

        j.Key("joints");
        assert(skin.joints.size() >= 1);
        j.StartArray();
        for (const auto& joint : skin.joints) {
            j.Int(joint);
        }
        j.EndArray();

        if (!skin.name.empty()) {
            j.Key("name");
            j.String(skin.name.c_str());
        }

        // todo: extensions / extras
        j.EndObject();
    }

    j.EndArray();
}