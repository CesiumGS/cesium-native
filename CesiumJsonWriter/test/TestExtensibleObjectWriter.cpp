#include "schema/TestObjectWriter.h"
#include <algorithm>
#include <catch2/catch.hpp>
#include <cstddef>
#include <gsl/span>
#include <iostream>
#include <rapidjson/document.h>
#include <string>

using namespace CesiumJsonWriter;

namespace {
Test::TestExtension createExtension() {
  Test::TestExtension ext;
  Test::Property p1, p2;

  p1.intOptional = 5;
  p1.floatArray = {1.1, 2.2, 3.3};
  p2.floatArray = {4.4, 5.5, 6.6};

  ext.properties = {p1, p2};

  return ext;
}

Test::TestObject createObject() {
  Test::TestObject obj;
  obj.intProperty = 10;
  obj.stringProperty = "test";

  Test::Property p1, p2;
  p1.intOptional = 5;
  p1.floatArray = {1.1, 2.2, 3.3};
  p2.floatArray = {4.4, 5.5, 6.6};
  obj.additionalProperties = {{"p1", p1}, {"p2", p2}};

  return obj;
}

void checkProperty(
    const rapidjson::Value& json,
    const Test::Property& property) {
  if (property.intOptional.has_value()) {
    CHECK(json.HasMember("intOptional"));
    CHECK(json["intOptional"] == *property.intOptional);
  } else {
    CHECK_FALSE(json.HasMember("intOptional"));
  }

  CHECK(json["floatArray"].Size() == property.floatArray.size());
  for (unsigned int i = 0; i < property.floatArray.size(); i++) {
    CHECK(json["floatArray"][i] == property.floatArray[i]);
  }
}

void writeAndCheck(
    const Test::TestObject& obj,
    rapidjson::Document& document,
    const ExtensionWriterContext& context) {
  JsonWriter jsonWriter;
  Test::TestObjectWriter::write(obj, jsonWriter, context);

  document.Parse(jsonWriter.toString().c_str());
  std::cout << jsonWriter.toString() << "\n";

  CHECK(document.IsObject());
  CHECK(document["intProperty"] == obj.intProperty);
  CHECK(document["stringProperty"].GetString() == obj.stringProperty);
  for (const auto& item : obj.additionalProperties) {
    checkProperty(document[item.first.c_str()], item.second);
  }
}
} // namespace

TEST_CASE("Write Extensible Object", "[JsonWriter]") {
  using namespace std::string_literals;

  Test::TestObject obj = createObject();

  ExtensionWriterContext context;
  rapidjson::Document document;
  writeAndCheck(obj, document, context);

  CHECK_FALSE(document.HasMember("extensions"));
}

TEST_CASE("Write Extensible Object with Extension", "[JsonWriter]") {
  using namespace std::string_literals;

  Test::TestObject obj = createObject();
  Test::TestExtension ext = createExtension();
  obj.extensions.emplace(Test::TestExtension::ExtensionName, ext);

  ExtensionWriterContext context;
  context.registerExtension<Test::TestObject, Test::TestExtensionWriter>();

  rapidjson::Document document;
  writeAndCheck(obj, document, context);

  CHECK(document.HasMember("extensions"));
  CHECK(document["extensions"].HasMember(Test::TestExtension::ExtensionName));

  const rapidjson::Value& extMember =
      document["extensions"][Test::TestExtension::ExtensionName];
  CHECK(extMember["properties"].Size() == ext.properties.size());
  for (unsigned int i = 0; i < ext.properties.size(); i++) {
    checkProperty(extMember["properties"][i], ext.properties[i]);
  }
}
