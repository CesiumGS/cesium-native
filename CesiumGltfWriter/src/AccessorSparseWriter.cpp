#include "AccessorSparseWriter.h"
#include "ExtensionWriter.h"
#include "JsonWriter.h"
#include <CesiumGltf/AccessorSparse.h>
#include <CesiumGltf/AccessorSparseIndices.h>
#include <CesiumGltf/AccessorSparseValues.h>
#include <JsonObjectWriter.h>
#include <magic_enum.hpp>
#include <stdexcept>

void writeAccessorSparseIndices(
    const CesiumGltf::AccessorSparseIndices& indices,
    CesiumGltf::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.Key("indices");
  j.StartObject();
  j.Key("bufferView");
  j.Int(indices.bufferView);
  if (indices.byteOffset >= 0) {
    j.Key("byteOffset");
    j.Int64(indices.byteOffset);
  }

  j.KeyPrimitive(
      "componentType",
      static_cast<std::uint16_t>(
          magic_enum::enum_integer(indices.componentType)));

  if (!indices.extensions.empty()) {
    writeExtensions(indices.extensions, j);
  }

  if (!indices.extras.empty()) {
    j.Key("extras");
    writeJsonValue(indices.extras, j, false);
  }
}

void writeAccessorSparseValues(
    const CesiumGltf::AccessorSparseValues& values,
    CesiumGltf::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.Key("values");
  j.StartObject();
  j.Key("bufferView");
  j.Int(values.bufferView);
  if (values.byteOffset >= 0) {
    j.Key("byteOffset");
    j.Int64(values.byteOffset);
  }

  if (!values.extensions.empty()) {
    writeExtensions(values.extensions, j);
  }

  if (!values.extras.empty()) {
    j.Key("extras");
    writeJsonValue(values.extras, j);
  }
}

void CesiumGltf::writeAccessorSparse(
    const AccessorSparse& accessorSparse,
    CesiumGltf::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.Key("sparse");
  j.StartObject();
  j.Key("count");
  j.Int64(accessorSparse.count);
  writeAccessorSparseIndices(accessorSparse.indices, jsonWriter);
  writeAccessorSparseValues(accessorSparse.values, jsonWriter);

  if (!accessorSparse.extensions.empty()) {
    writeExtensions(accessorSparse.extensions, jsonWriter);
  }

  if (!accessorSparse.extras.empty()) {
    j.Key("extras");
    writeJsonValue(accessorSparse.extras, jsonWriter);
  }

  j.EndObject();
}
