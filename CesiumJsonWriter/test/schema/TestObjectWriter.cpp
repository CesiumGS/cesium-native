#include "TestObjectWriter.h"

#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumJsonWriter/ExtensionsWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>

using namespace CesiumJsonWriter;
using namespace CesiumUtility;

namespace Test {

namespace {
[[maybe_unused]] void writeJson(
    bool val,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& /* context */) {
  jsonWriter.Bool(val);
}

[[maybe_unused]] void writeJson(
    int64_t val,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& /* context */) {
  jsonWriter.Int64(val);
}

[[maybe_unused]] void writeJson(
    double val,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& /* context */) {
  jsonWriter.Double(val);
}

[[maybe_unused]] void writeJson(
    const std::string& val,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& /* context */) {
  jsonWriter.String(val);
}

template <typename T>
void writeJson(
    const std::vector<T>& list,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context);

template <typename T>
void writeJson(
    const std::optional<T>& list,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context);

template <typename T>
void writeJson(
    const std::unordered_map<std::string, T>& list,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context);
} // namespace

/////////////////////////
// Writer for Property
/////////////////////////

namespace {
void writeJson(
    const Property& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  jsonWriter.StartObject();
  if (obj.intOptional.has_value()) {
    jsonWriter.Key("intOptional");
    writeJson(obj.intOptional, jsonWriter, context);
  }
  jsonWriter.Key("floatArray");
  writeJson(obj.floatArray, jsonWriter, context);
  jsonWriter.EndObject();
}
} // namespace

void PropertyWriter::write(
    const Property& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  writeJson(obj, jsonWriter, context);
}

/////////////////////////
// Writer for TestExtension
/////////////////////////
//
namespace {
void writeJson(
    const TestExtension& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  jsonWriter.StartObject();
  jsonWriter.Key("properties");
  writeJson(obj.properties, jsonWriter, context);
  jsonWriter.EndObject();
}
} // namespace

void TestExtensionWriter::write(
    const TestExtension& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  writeJson(obj, jsonWriter, context);
}

/////////////////////////
// Writer for TestObject
/////////////////////////
//
namespace {
void writeJson(
    const TestObject& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  jsonWriter.StartObject();

  jsonWriter.Key("intProperty");
  writeJson(obj.intProperty, jsonWriter, context);

  jsonWriter.Key("stringProperty");
  writeJson(obj.stringProperty, jsonWriter, context);

  for (const auto& item : obj.additionalProperties) {
    jsonWriter.Key(item.first);
    writeJson(item.second, jsonWriter, context);
  }

  if (!obj.extensions.empty()) {
    jsonWriter.Key("extensions");
    writeJsonExtensions(obj, jsonWriter, context);
  }

  jsonWriter.EndObject();
}
} // namespace

void TestObjectWriter::write(
    const TestObject& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  writeJson(obj, jsonWriter, context);
}

////////////////////////
// template primitive writer implementations
////////////////////////

namespace {
template <typename T>
[[maybe_unused]] void writeJson(
    const std::vector<T>& list,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  jsonWriter.StartArray();
  for (const T& item : list) {
    writeJson(item, jsonWriter, context);
  }
  jsonWriter.EndArray();
}

template <typename T>
[[maybe_unused]] void writeJson(
    const std::optional<T>& val,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  if (val.has_value()) {
    writeJson(*val, jsonWriter, context);
  } else {
    jsonWriter.Null();
  }
}

template <typename T>
[[maybe_unused]] void writeJson(
    const std::unordered_map<std::string, T>& obj,
    JsonWriter& jsonWriter,
    const ExtensionWriterContext& context) {
  for (const auto& item : obj) {
    jsonWriter.Key(item.first);
    writeJson(item.second, jsonWriter, context);
  }
}
} // namespace

} // namespace Test
