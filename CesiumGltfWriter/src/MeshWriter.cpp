#include "MeshWriter.h"
#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include "JsonWriter.h"
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <magic_enum.hpp>
#include <vector>

void writePrimitive(
    const CesiumGltf::MeshPrimitive& primitive,
    CesiumGltf::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.StartObject();

  j.Key("attributes");
  assert(!primitive.attributes.empty());
  j.StartObject();
  for (const auto& [key, index] : primitive.attributes) {
    j.KeyPrimitive(key, index);
  }
  j.EndObject();

  if (primitive.indices >= 0) {
    j.Key("indices");
    j.Int(primitive.indices);
  }

  if (primitive.material >= 0) {
    j.Key("material");
    j.Int(primitive.material);
  }

  if (primitive.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLES) {
    j.Key("mode");
    j.Int(magic_enum::enum_integer(primitive.mode));
  }

  if (!primitive.targets.empty()) {
    j.Key("targets");
    j.StartArray();
    for (const auto& target : primitive.targets) {
      j.StartObject();
      for (const auto& [name, index] : target) {
        j.Key(name.c_str());
        j.Int(index);
      }
      j.EndObject();
    }
    j.EndArray();
  }

  if (!primitive.extensions.empty()) {
    j.Key("extensions");
    writeExtensions(primitive.extensions, j);
  }

  if (!primitive.extras.empty()) {
    j.Key("extras");
    CesiumGltf::writeJsonValue(primitive.extras, j, false);
  }
  j.EndObject();
}

void CesiumGltf::writeMesh(
    const std::vector<Mesh>& meshes,
    CesiumGltf::JsonWriter& jsonWriter) {

  if (meshes.empty()) {
    return;
  }

  auto& j = jsonWriter;
  j.Key("meshes");
  j.StartArray();
  for (const auto& mesh : meshes) {
    j.StartObject();

    assert(!mesh.primitives.empty());
    j.Key("primitives");
    j.StartArray();
    for (const auto& primitive : mesh.primitives) {
      writePrimitive(primitive, j);
    }
    j.EndArray();

    if (!mesh.weights.empty()) {
      j.Key("weights");
      j.StartArray();
      for (const auto& weight : mesh.weights) {
        j.Double(weight);
      }
      j.EndArray();
    }

    if (!mesh.name.empty()) {
      j.Key("name");
      j.String(mesh.name.c_str());
    }

    if (!mesh.extensions.empty()) {
      writeExtensions(mesh.extensions, j);
    }

    if (!mesh.extras.empty()) {
      j.Key("extras");
      writeJsonValue(mesh.extras, j, false);
    }

    j.EndObject();
  }

  j.EndArray();
}
