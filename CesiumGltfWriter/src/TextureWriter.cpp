#include "TextureWriter.h"

#include "ExtensionWriter.h"

#include <CesiumJsonWriter/JsonObjectWriter.h>

void CesiumGltf::writeTexture(
    const std::vector<Texture>& textures,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  if (textures.empty()) {
    return;
  }

  auto& j = jsonWriter;

  j.Key("textures");
  j.StartArray();
  for (const auto& texture : textures) {
    j.StartObject();

    if (texture.sampler >= 0) {
      j.Key("sampler");
      j.Int(texture.sampler);
    }

    if (texture.source >= 0) {
      j.Key("source");
      j.Int(texture.source);
    }

    if (!texture.name.empty()) {
      j.Key("name");
      j.String(texture.name.c_str());
    }

    if (!texture.extensions.empty()) {
      writeExtensions(texture.extensions, j);
    }

    if (!texture.extras.empty()) {
      j.Key("extras");
      writeJsonValue(texture.extras, j);
    }

    j.EndObject();
  }
  j.EndArray();
}
