#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumUtility/JsonValue.h>

#include <utility>

namespace CesiumJsonWriter {
namespace {
void primitiveWriter(const CesiumUtility::JsonValue& item, JsonWriter& j);
void recursiveArrayWriter(
    const CesiumUtility::JsonValue::Array& array,
    JsonWriter& j);
void recursiveObjectWriter(
    const CesiumUtility::JsonValue::Object& object,
    JsonWriter& j);

void primitiveWriter(const CesiumUtility::JsonValue& item, JsonWriter& j) {
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
    const CesiumUtility::JsonValue::Array& array,
    JsonWriter& j) {
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
    const CesiumUtility::JsonValue::Object& object,
    JsonWriter& j) {

  j.StartObject();

  for (const auto& [key, item] : object) {
    j.Key(key);
    if (item.isArray()) {
      recursiveArrayWriter(
          std::get<CesiumUtility::JsonValue::Array>(item.value),
          j);
    }

    if (item.isObject()) {
      recursiveObjectWriter(
          std::get<CesiumUtility::JsonValue::Object>(item.value),
          j);
    }

    else {
      primitiveWriter(item, j);
    }
  }

  j.EndObject();
}
} // namespace

void writeJsonValue(
    const CesiumUtility::JsonValue& value,
    JsonWriter& jsonWriter) {

  if (value.isArray()) {
    recursiveArrayWriter(
        std::get<CesiumUtility::JsonValue::Array>(value.value),
        jsonWriter);
  } else if (value.isObject()) {
    recursiveObjectWriter(
        std::get<CesiumUtility::JsonValue::Object>(value.value),
        jsonWriter);
  } else {
    primitiveWriter(value, jsonWriter);
  }
}
} // namespace CesiumJsonWriter
