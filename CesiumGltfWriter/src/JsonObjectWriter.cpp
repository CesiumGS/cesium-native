#include "JsonObjectWriter.h"
#include <CesiumGltf/JsonValue.h>
#include <JsonWriter.h>
#include <functional>
#include <stack>
#include <utility>

using namespace CesiumGltf;
using namespace rapidjson;

void primitiveWriter(const JsonValue& item, CesiumGltf::JsonWriter& j);
void recursiveArrayWriter(
    const JsonValue::Array& array,
    CesiumGltf::JsonWriter& j);
void recursiveObjectWriter(
    const JsonValue::Object& object,
    CesiumGltf::JsonWriter& j,
    bool isRoot = false);

void primitiveWriter(const JsonValue& item, CesiumGltf::JsonWriter& j) {
    if (item.isBool()) {
        j.Bool(std::get<JsonValue::Bool>(item.value));
    }

    else if (item.isNull()) {
        j.Null();
    }

    else if (item.isNumber()) {
        j.Double(std::get<JsonValue::Number>(item.value));
    }

    else if (item.isString()) {
        j.String(std::get<JsonValue::String>(item.value).c_str());
    }
}

void recursiveArrayWriter(
    const JsonValue::Array& array,
    CesiumGltf::JsonWriter& j) {
    j.StartArray();
    for (const auto& item : array) {
        if (item.isArray()) {
            recursiveArrayWriter(std::get<JsonValue::Array>(item.value), j);
        }

        else if (item.isObject()) {
            recursiveObjectWriter(std::get<JsonValue::Object>(item.value), j);
        }

        else {
            primitiveWriter(item, j);
        }
    }
    j.EndArray();
}

void recursiveObjectWriter(
    const CesiumGltf::JsonValue::Object& object,
    CesiumGltf::JsonWriter& j,
    bool isRoot) {

    if (!isRoot)  {
        j.StartObject();
    }

    for (const auto& [key, item] : object) {
        j.Key(key.c_str());
        if (item.isArray()) {
            recursiveArrayWriter(std::get<JsonValue::Array>(item.value), j);
        }

        if (item.isObject()) {
            recursiveObjectWriter(std::get<JsonValue::Object>(item.value), j);
        }

        else {
            primitiveWriter(item, j);
        }
    }

    if (!isRoot) {
        j.EndObject();
    }
}

void CesiumGltf::writeJsonValue(
    const JsonValue& value,
    CesiumGltf::JsonWriter& jsonWriter) {
    if (value.isArray()) {
        recursiveArrayWriter(std::get<JsonValue::Array>(value.value), jsonWriter);
    } else if (value.isObject()) {
        recursiveObjectWriter(std::get<JsonValue::Object>(value.value), jsonWriter, true);
    } else {
        primitiveWriter(value, jsonWriter);
    }
}