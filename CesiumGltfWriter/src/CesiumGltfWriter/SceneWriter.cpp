#include "SceneWriter.h"

#include "ExtensionWriter.h"

#include <CesiumJsonWriter/JsonObjectWriter.h>

void CesiumGltfWriter::writeScene(
    const std::vector<CesiumGltf::Scene>& scenes,
    CesiumJsonWriter::JsonWriter& jsonWriter) {

  if (scenes.empty()) {
    return;
  }

  auto& j = jsonWriter;

  j.Key("scenes");
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

    if (!scene.extensions.empty()) {
      writeExtensions(scene.extensions, j);
    }

    if (!scene.extras.empty()) {
      j.Key("extras");
      writeJsonValue(scene.extras, j);
    }

    j.EndObject();
  }

  j.EndArray();
}
