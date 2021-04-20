#include "BufferWriter.h"
#include "JsonWriter.h"
#include "PrettyJsonWriter.h"
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/WriteModelOptions.h>
#include <CesiumGltf/WriteModelResult.h>
#include <CesiumUtility/JsonValue.h>
#include <catch2/catch.hpp>
#include <rapidjson/document.h>

const std::vector<std::byte> HELLO_WORLD_STR{
    std::byte('H'),
    std::byte('e'),
    std::byte('l'),
    std::byte('l'),
    std::byte('o'),
    std::byte('W'),
    std::byte('o'),
    std::byte('r'),
    std::byte('l'),
    std::byte('d'),
    std::byte('!')};

TEST_CASE(
    "BufferWriter automatically converts buffer.cesium.data to base64 if "
    "autoConvertConvertDataToBase64 is specified",
    "[GltfWriter]") {

  CesiumGltf::Buffer buffer;
  buffer.cesium.data = HELLO_WORLD_STR;
  CesiumGltf::PrettyJsonWriter writer;

  // Intentionally set an erroneous byte size, the writer should ignore it.
  // if a base64 conversion occured.
  buffer.byteLength = 1337;

  CesiumGltf::WriteModelOptions options;
  options.exportType = CesiumGltf::GltfExportType::GLTF;
  options.autoConvertDataToBase64 = true;

  writer.StartObject();
  CesiumGltf::WriteModelResult result;
  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);

  writer.EndObject();

  REQUIRE(result.errors.empty());
  REQUIRE(result.warnings.empty());

  rapidjson::Document document;
  document.Parse(writer.toString().c_str());
  REQUIRE(document.IsObject());
  REQUIRE(document["buffers"].IsArray());
  const auto& buffers = document["buffers"];
  const auto& bufferArray = buffers.GetArray();
  const auto& firstBuffer = bufferArray[0];
  REQUIRE(firstBuffer.IsObject());
  // 'HelloWorld!' in base64
  const auto uri = std::string(firstBuffer["uri"].GetString());
  const auto byteLength = firstBuffer["byteLength"].GetInt();
  REQUIRE(uri == "data:application/octet-stream;base64,SGVsbG9Xb3JsZCE=");
  REQUIRE(byteLength == 11);
}

TEST_CASE(
    "If external file URI is detected for buffer[0] in glTF mode, user "
    "provided file io lambda is invoked",
    "[GltfWriter]") {
  CesiumGltf::Buffer buffer;
  buffer.uri = "helloworld.bin";
  buffer.cesium.data = HELLO_WORLD_STR;

  bool callbackInvoked = false;
  const auto onHelloWorldBin = [&callbackInvoked, &buffer](
                                   std::string_view filename,
                                   const std::vector<std::byte>& bytes) {
    REQUIRE(filename == *buffer.uri);
    REQUIRE(bytes == buffer.cesium.data);
    callbackInvoked = true;
  };

  CesiumGltf::PrettyJsonWriter writer;

  // Intentionally set an erroneous byte size, the writer should ignore it if
  // writing to an external file would occur.
  buffer.byteLength = 1337;

  CesiumGltf::WriteModelOptions options;
  options.exportType = CesiumGltf::GltfExportType::GLTF;
  options.autoConvertDataToBase64 = true;

  CesiumGltf::WriteModelResult result;

  writer.StartObject();
  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options,
      onHelloWorldBin);
  writer.EndObject();

  REQUIRE(result.errors.empty());
  REQUIRE(result.warnings.empty());
  REQUIRE(callbackInvoked);

  rapidjson::Document document;
  document.Parse(writer.toString().c_str());
  REQUIRE(document.IsObject());
  REQUIRE(document["buffers"].IsArray());
  const auto& buffers = document["buffers"];
  const auto& bufferArray = buffers.GetArray();
  const auto& firstBuffer = bufferArray[0];
  REQUIRE(firstBuffer.IsObject());
  // 'HelloWorld!' in base64
  const auto uri = std::string(firstBuffer["uri"].GetString());
  const auto byteLength =
      static_cast<std::size_t>(firstBuffer["byteLength"].GetInt());
  REQUIRE(uri == buffer.uri);
  REQUIRE(byteLength == buffer.cesium.data.size());
}

TEST_CASE("Buffer that only has byteLength set is serialized correctly") {
  CesiumGltf::Buffer buffer;
  buffer.byteLength = 1234;
  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::WriteModelResult result;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);

  REQUIRE(result.errors.empty());
  REQUIRE(result.warnings.empty());

  writer.EndObject();
  const auto asStringView = writer.toStringView();
  REQUIRE(asStringView == R"({"buffers":[{"byteLength":1234}]})");
}

TEST_CASE("URI zero CANNOT be set in GLB mode. (0th buffer is reserved as "
          "binary chunk).") {
  CesiumGltf::Buffer buffer;
  CesiumGltf::JsonWriter writer;
  buffer.uri = "literally anything here should trigger this error";
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLB;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.size() == 1);

  const auto& errorString = result.errors.at(0);
  REQUIRE(errorString.rfind("URIErroneouslyDefined", 0) == 0);
}

TEST_CASE("If uri is NOT set and buffer.cesium.data is NOT empty and "
          "AutoConvertDataToBase64 is NOT set, then user provided lambda with "
          "bufferIndex.bin name should be called") {
  CesiumGltf::Buffer buffer;
  CesiumGltf::JsonWriter writer;
  buffer.cesium.data = HELLO_WORLD_STR;

  bool callbackInvoked = false;
  const auto onHelloWorldBin = [&callbackInvoked, &buffer](
                                   std::string_view filename,
                                   const std::vector<std::byte>& bytes) {
    REQUIRE(filename == "0.bin");
    REQUIRE(bytes == buffer.cesium.data);
    callbackInvoked = true;
  };

  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options,
      onHelloWorldBin);
  writer.EndObject();

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.empty());
  REQUIRE(callbackInvoked);
}

TEST_CASE("AmbiguiousDataSource error returned if buffer.uri is set to base64 "
          "uri and "
          "buffer.cesium.data also set") {
  CesiumGltf::Buffer buffer;
  buffer.uri = "data:application/octet-stream;base64,SGVsbG9Xb3JsZCE=";
  buffer.cesium.data = HELLO_WORLD_STR;
  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.size() == 1);

  const auto& errorString = result.errors.at(0);
  REQUIRE(errorString.rfind("AmbiguiousDataSource", 0) == 0);
}

TEST_CASE("buffer.uri is passed through to final json string if appropriate") {
  CesiumGltf::Buffer buffer;
  buffer.uri = "data:application/octet-stream;base64,SGVsbG9Xb3JsZCE=";
  buffer.byteLength = static_cast<std::int64_t>(HELLO_WORLD_STR.size());
  buffer.name = "HelloWorldBuffer";
  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options),
      writer.EndObject();

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.empty());

  rapidjson::Document document;
  document.Parse(writer.toString().c_str());
  REQUIRE(document.IsObject());
  REQUIRE(document["buffers"].IsArray());
  const auto& buffers = document["buffers"];
  const auto& bufferArray = buffers.GetArray();
  const auto& firstBuffer = bufferArray[0];
  REQUIRE(firstBuffer.IsObject());
  // 'HelloWorld!' in base64
  const auto uri = std::string(firstBuffer["uri"].GetString());
  const auto byteLength =
      static_cast<std::size_t>(firstBuffer["byteLength"].GetInt());
  const auto name = std::string(firstBuffer["name"].GetString());
  REQUIRE(uri == buffer.uri);

  REQUIRE(byteLength == HELLO_WORLD_STR.size());
  REQUIRE(name == "HelloWorldBuffer");
}

TEST_CASE("base64 uri set but byte length not set") {
  CesiumGltf::Buffer buffer;
  buffer.uri = "data:application/octet-stream;base64,SGVsbG9Xb3JsZCE=";
  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.size() == 1);
  const auto& errorString = result.errors.at(0);
  REQUIRE(errorString.rfind("ByteLengthNotSet", 0) == 0);
}

TEST_CASE("If writing in GLB mode, buffer[0] automatically has its byteLength "
          "calculated based off buffer.cesium.data") {
  CesiumGltf::Buffer buffer;
  buffer.cesium.data = HELLO_WORLD_STR;
  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLB;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);
  writer.EndObject();

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.empty());

  rapidjson::Document document;
  auto string = writer.toString();
  document.Parse(string.c_str());
  REQUIRE(document.IsObject());
  REQUIRE(document["buffers"].IsArray());

  const auto& buffers = document["buffers"];
  const auto& bufferArray = buffers.GetArray();
  const auto& firstBuffer = bufferArray[0];
  REQUIRE(firstBuffer.IsObject());
  const auto byteLength =
      static_cast<std::size_t>(firstBuffer["byteLength"].GetInt());
  REQUIRE(!firstBuffer.HasMember("uri"));
  REQUIRE(!firstBuffer.HasMember("name"));
  REQUIRE(byteLength == HELLO_WORLD_STR.size());
}

TEST_CASE("MissingDataSource error returned if ExternalFileURI detected and "
          "buffer.cesium.data is empty") {
  CesiumGltf::Buffer buffer;
  buffer.uri = "Foobar.bin";
  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.size() == 1);
  const auto& errorString = result.errors.at(0);
  REQUIRE(errorString.rfind("MissingDataSource", 0) == 0);
}

TEST_CASE("extras and extensions are detected and serialized") {
  CesiumGltf::Buffer buffer;
  buffer.extras = CesiumUtility::JsonValue::Object{
      {"some", CesiumUtility::JsonValue("extra")}};

  CesiumUtility::JsonValue testExtension("value");

  buffer.extensions.emplace("key", testExtension);

  CesiumGltf::JsonWriter writer;
  writer.StartObject();

  CesiumGltf::WriteModelOptions options;
  CesiumGltf::WriteModelResult result;
  options.exportType = CesiumGltf::GltfExportType::GLTF;

  CesiumGltf::writeBuffer(
      result,
      std::vector<CesiumGltf::Buffer>{buffer},
      writer,
      options);
  writer.EndObject();

  REQUIRE(result.warnings.empty());
  REQUIRE(result.errors.empty());

  rapidjson::Document document;
  auto string = writer.toString();
  document.Parse(string.c_str());

  const auto& buffers = document["buffers"];
  const auto& bufferArray = buffers.GetArray();
  const auto& firstBuffer = bufferArray[0];
  REQUIRE(firstBuffer.IsObject());
  REQUIRE(firstBuffer.HasMember("extras"));
  REQUIRE(firstBuffer.HasMember("extensions"));
  REQUIRE(std::string(firstBuffer["extras"]["some"].GetString()) == "extra");
  REQUIRE(std::string(firstBuffer["extensions"]["key"].GetString()) == "value");
}
