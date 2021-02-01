#include "AccessorSparseWriter.h"
#include <CesiumGltf/AccessorSparse.h>
#include <CesiumGltf/AccessorSparseIndices.h>
#include <CesiumGltf/AccessorSparseValues.h>
#include <rapidjson/stringbuffer.h>
#include <stdexcept>

void writeAccessorSparseIndices(
    const CesiumGltf::AccessorSparseIndices& indices,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("indices");
    j.StartObject();
    j.Key("bufferView");
    j.Int(indices.bufferView);
    if (indices.byteOffset >= 0) {
        j.Key("byteOffset");
        j.Int64(indices.byteOffset);
    }
    j.Key("componentType");
    j.Int(static_cast<std::underlying_type<
              CesiumGltf::AccessorSparseIndices::ComponentType>::type>(
        indices.componentType));

    if (!indices.extensions.empty()) {
        // every potential type of extension will need a
        // 'WriteXXXExtension(extension, jsonWriter)'
        // method. We'll skip that for now.
        throw std::runtime_error("Not implemented.");
    }

    if (!indices.extras.empty()) {
        throw std::runtime_error("Not implemented.");
    }
}

void writeAccessorSparseValues(
    const CesiumGltf::AccessorSparseValues& values,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
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
        // every potential type of extension will need a
        // 'WriteXXXExtension(extension, jsonWriter)'
        // method. We'll skip that for now.
        throw std::runtime_error("Not implemented.");
    }

    if (!values.extras.empty()) {
        throw std::runtime_error("Not implemented.");
    }
}

void CesiumGltf::writeAccessorSparse(
    const AccessorSparse& accessorSparse,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
    auto& j = jsonWriter;
    j.Key("sparse");
    j.StartObject();
    j.Key("count");
    j.Int64(accessorSparse.count);
    writeAccessorSparseIndices(accessorSparse.indices, jsonWriter);
    writeAccessorSparseValues(accessorSparse.values, jsonWriter);
    j.EndObject();
}