#include "CesiumGltfWriter/AccessorSparseWriter.h"

#include "CesiumGltfWriter/ExtensionWriter.h"

#include <CesiumGltf/AccessorSparse.h>
#include <CesiumGltf/AccessorSparseIndices.h>
#include <CesiumGltf/AccessorSparseValues.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <stdexcept>

namespace CesiumGltfWriter {
namespace {
void writeAccessorSparseIndices(
    const CesiumGltf::AccessorSparseIndices& indices,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.Key("indices");
  j.StartObject();
  j.Key("bufferView");
  j.Int(indices.bufferView);
  if (indices.byteOffset >= 0) {
    j.Key("byteOffset");
    j.Int64(indices.byteOffset);
  }

  j.KeyPrimitive("componentType", indices.componentType);

  if (!indices.extensions.empty()) {
    writeExtensions(indices.extensions, j);
  }

  if (!indices.extras.empty()) {
    j.Key("extras");
    CesiumJsonWriter::writeJsonValue(indices.extras, j);
  }
}

void writeAccessorSparseValues(
    const CesiumGltf::AccessorSparseValues& values,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
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
    CesiumJsonWriter::writeJsonValue(values.extras, j);
  }
}
} // namespace

void writeAccessorSparse(
    const CesiumGltf::AccessorSparse& accessorSparse,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
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
    CesiumJsonWriter::writeJsonValue(accessorSparse.extras, jsonWriter);
  }

  j.EndObject();
}
} // namespace CesiumGltfWriter