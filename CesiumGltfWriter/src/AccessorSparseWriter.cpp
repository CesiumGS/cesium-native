#include "JsonWriter.h"
#include "AccessorSparseWriter.h"
#include <CesiumGltf/AccessorSparse.h>
#include <CesiumGltf/AccessorSparseIndices.h>
#include <CesiumGltf/AccessorSparseValues.h>
#include <JsonObjectWriter.h>
#include <stdexcept>
#include <magic_enum.hpp>

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

    j.KeyPrimitive("componentType", static_cast<std::uint16_t>(magic_enum::enum_integer(indices.componentType)));

    if (!indices.extensions.empty()) {
        throw std::runtime_error("Not implemented.");
    }

    if (!indices.extras.empty()) {
        j.Key("extras");
        writeJsonObject(indices.extras, j);
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
        throw std::runtime_error("Not implemented.");
    }

    if (!values.extras.empty()) {
        j.Key("extras");
        writeJsonObject(values.extras, j);
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
    j.EndObject();
}