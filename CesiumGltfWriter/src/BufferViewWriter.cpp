#include "BufferViewWriter.h"
#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include <magic_enum.hpp>
#include <stdexcept>
#include <type_traits>

void CesiumGltf::writeBufferView(
    const std::vector<BufferView>& bufferViews,
    CesiumGltf::JsonWriter& jsonWriter) {

  if (bufferViews.empty()) {
    return;
  }

  auto& j = jsonWriter;
  j.Key("bufferViews");
  j.StartArray();
  for (const auto& bufferView : bufferViews) {
    j.StartObject();

    j.Key("buffer");
    j.Int(bufferView.buffer);

    if (bufferView.byteOffset >= 0) {
      j.Key("byteOffset");
      j.Int64(bufferView.byteOffset);
    }

    j.Key("byteLength");
    j.Int64(bufferView.byteLength);

    if (bufferView.byteStride) {
      j.Key("byteStride");
      j.Int64(*bufferView.byteStride);
    }

    if (bufferView.target) {
      j.Key("target");
      j.Int(magic_enum::enum_integer(*bufferView.target));
    }

    if (!bufferView.name.empty()) {
      j.Key("name");
      j.String(bufferView.name);
    }

    if (!bufferView.extensions.empty()) {
      writeExtensions(bufferView.extensions, j);
    }

    if (!bufferView.extras.empty()) {
      j.Key("extras");
      writeJsonValue(bufferView.extras, j);
    }

    j.EndObject();
  }
  j.EndArray();
}
