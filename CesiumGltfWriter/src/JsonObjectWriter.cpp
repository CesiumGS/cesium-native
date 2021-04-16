#include "JsonObjectWriter.h"
#include <CesiumUtility/JsonValue.h>
#include <JsonWriter.h>
#include <functional>
#include <stack>
#include <utility>

using namespace CesiumGltf;
using namespace CesiumUtility;
using namespace rapidjson;

void primitiveWriter(const JsonValue& item, CesiumGltf::JsonWriter& j);
void recursiveArrayWriter(
    const JsonValue::Array& array,
    CesiumGltf::JsonWriter& j);
void recursiveObjectWriter(
    const JsonValue::Object& object,
    CesiumGltf::JsonWriter& j,
    bool hasRootObject = false);

void primitiveWriter(const JsonValue& item, CesiumGltf::JsonWriter& j) {
  if (item.isBool()) {
    j.Bool(item.getBool());
  }

  else if (item.isNull()) {
    j.Null();
  }

  else if (item.isInt64()) {
    j.Int64(item.getInt64());
  }

  else if (item.isUint64()) {
    j.Uint64(item.getUint64());
  }

  else if (item.isDouble()) {
    j.Double(item.getDouble());
  }

  else if (item.isString()) {
    j.String(item.getString());
  }
}

void recursiveArrayWriter(
    const JsonValue::Array& array,
    CesiumGltf::JsonWriter& j) {
  j.StartArray();
  for (const auto& item : array) {
    if (item.isArray()) {
      recursiveArrayWriter(item.getArray(), j);
    }

    else if (item.isObject()) {
      recursiveObjectWriter(item.getObject(), j);
    }

    else {
      primitiveWriter(item, j);
    }
  }
  j.EndArray();
}

void recursiveObjectWriter(
    const JsonValue::Object& object,
    CesiumGltf::JsonWriter& j,
    bool hasRootObject) {

  if (!hasRootObject) {
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

  if (!hasRootObject) {
    j.EndObject();
  }
}

void CesiumGltf::writeJsonValue(
    const JsonValue& value,
    CesiumGltf::JsonWriter& jsonWriter,
    bool hasRootObject) {
  if (value.isArray()) {
    recursiveArrayWriter(std::get<JsonValue::Array>(value.value), jsonWriter);
  } else if (value.isObject()) {
    recursiveObjectWriter(
        std::get<JsonValue::Object>(value.value),
        jsonWriter,
        hasRootObject);
  } else {
    primitiveWriter(value, jsonWriter);
  }
}
