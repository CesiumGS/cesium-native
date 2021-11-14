#include "CesiumGltfWriter/BufferViewWriter.h"

#include "CesiumGltfWriter/ExtensionWriter.h"

#include <CesiumGltf/BufferView.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <stdexcept>
#include <type_traits>

namespace CesiumGltfWriter {

void writeBufferView(
    const std::vector<CesiumGltf::BufferView>& bufferViews,
    CesiumJsonWriter::JsonWriter& jsonWriter) {

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
      j.Int(*bufferView.target);
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
} // namespace CesiumGltfWriter