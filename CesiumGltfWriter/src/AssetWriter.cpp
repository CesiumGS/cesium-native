#include "AssetWriter.h"
#include "ExtensionWriter.h"
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumGltf/Asset.h>

void CesiumGltf::writeAsset(
    const Asset& asset,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.Key("asset");
  j.StartObject();

  if (asset.copyright) {
    j.KeyPrimitive("copyright", *asset.copyright);
  }

  if (asset.generator) {
    j.KeyPrimitive("generator", *asset.generator);
  }

  j.KeyPrimitive("version", asset.version);

  if (asset.minVersion) {
    j.KeyPrimitive("minVersion", *asset.minVersion);
  }

  if (!asset.extensions.empty()) {
    writeExtensions(asset.extensions, j);
  }

  if (!asset.extras.empty()) {
    j.Key("extras");
    writeJsonValue(asset.extras, j);
  }

  j.EndObject();
}
