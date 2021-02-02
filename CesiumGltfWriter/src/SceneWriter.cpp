#include "SceneWriter.h"
void CesiumGltf::writeScene(
    const std::vector<Scene>& scenes,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {

    if (scenes.empty()) {
        return;
    }

    auto& j = jsonWriter;

    j.Key("scene");
    j.StartArray();
    for (const auto& scene : scenes) {
        j.StartObject();
        if (!scene.nodes.empty()) {
            j.Key("nodes");
            j.StartArray();
            for (const auto& node : scene.nodes) {
                j.Int(node);
            }
            j.EndArray();
        }

        if (!scene.name.empty()) {
            j.Key("name");
            j.String(scene.name.c_str());
        }

        // todo: extensions / extras

        j.EndObject();
    }

    j.EndArray();
}