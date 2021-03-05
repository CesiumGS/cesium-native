#include "BufferWriter.h"
#include "JsonWriter.h"
#include "PrettyJsonWriter.h"
#include <CesiumGltf/JsonValue.h>
#include <CesiumGltf/WriterException.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/WriteFlags.h>
#include <catch2/catch.hpp>
#include <rapidjson/document.h>

const std::vector<std::uint8_t> HELLO_WORLD_STR {
    'H', 'e', 'l', 'l', 'o', 'W','o','r','l','d','!'
};

TEST_CASE("BufferWriter automatically converts buffer.cesium.data to base64 if WriteFlags::AutoConvertConvertDataToBase64 is set", "[GltfWriter]") {

    CesiumGltf::Buffer buffer;
    buffer.cesium.data = HELLO_WORLD_STR;
    CesiumGltf::PrettyJsonWriter writer;

    // Intentionally set an erroneous byte size, the writer should ignore it.
    // if a base64 conversion occured.
    buffer.byteLength = 1337;

    writer.StartObject();
    CesiumGltf::writeBuffer(
        std::vector<CesiumGltf::Buffer> { buffer }, 
        writer, 
        CesiumGltf::WriteFlags::GLTF | CesiumGltf::WriteFlags::AutoConvertDataToBase64
    );

    writer.EndObject();

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

TEST_CASE("If external file URI is detected for buffer[0] in glTF mode, user provided file io lambda is invoked", "[GltfWriter]") {
    CesiumGltf::Buffer buffer;
    buffer.uri = "helloworld.bin";
    buffer.cesium.data = HELLO_WORLD_STR;

    bool callbackInvoked = false;
    const auto onHelloWorldBin = [&callbackInvoked, &buffer](std::string_view filename, const std::vector<std::uint8_t>& bytes) {
        REQUIRE(filename == *buffer.uri);
        REQUIRE(bytes == buffer.cesium.data);
        callbackInvoked = true;
    };

    CesiumGltf::PrettyJsonWriter writer;

    // Intentionally set an erroneous byte size, the writer should ignore it if writing to
    // an external file would occur.
    buffer.byteLength = 1337;

    writer.StartObject();
    CesiumGltf::writeBuffer(
        std::vector<CesiumGltf::Buffer> { buffer }, 
        writer, 
        CesiumGltf::WriteFlags::GLTF | CesiumGltf::WriteFlags::AutoConvertDataToBase64,
        onHelloWorldBin
    );
    writer.EndObject();
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
    const auto byteLength = static_cast<std::size_t>(firstBuffer["byteLength"].GetInt());
    REQUIRE(uri == buffer.uri);
    REQUIRE(byteLength == buffer.cesium.data.size());
}

TEST_CASE("Buffer that only has byteLength set is serialized correctly") {
    CesiumGltf::Buffer buffer;
    buffer.byteLength = 1234;
    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF);
    writer.EndObject();
    const auto asStringView = writer.toStringView();
    REQUIRE(asStringView == R"({"buffers":[{"byteLength":1234}]})");
}

TEST_CASE("URI zero CANNOT be set in GLB mode. (0th buffer is reserved as binary chunk).") {
    CesiumGltf::Buffer buffer;
    CesiumGltf::JsonWriter writer;
    buffer.uri = "literally anything here should trigger this error";
    writer.StartObject();
    REQUIRE_THROWS_AS( 
        CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLB),
        CesiumGltf::URIErroneouslyDefined
    );
}

TEST_CASE("If uri is NOT set and buffer.cesium.data is NOT empty and AutoConvertDataToBase64 is NOT set, then user provided lambda with bufferIndex.bin name should be called") {
    CesiumGltf::Buffer buffer;
    CesiumGltf::JsonWriter writer;
    buffer.cesium.data = HELLO_WORLD_STR;

    bool callbackInvoked = false;
    const auto onHelloWorldBin = [&callbackInvoked, &buffer](std::string_view filename, const std::vector<std::uint8_t>& bytes) {
        REQUIRE(filename == "0.bin");
        REQUIRE(bytes == buffer.cesium.data);
        callbackInvoked = true;
    };

    writer.StartObject();
    CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF, onHelloWorldBin);
    writer.EndObject();
}

TEST_CASE("AmbiguiousDataSource thrown if buffer.uri is set to base64 uri and buffer.cesium.data also set") {
    CesiumGltf::Buffer buffer;
    buffer.uri = "data:application/octet-stream;base64,SGVsbG9Xb3JsZCE=";
    buffer.cesium.data = HELLO_WORLD_STR;
    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    REQUIRE_THROWS_AS(
        CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF),
        CesiumGltf::AmbiguiousDataSource
    );
}

TEST_CASE("buffer.uri is passed through to final json string if appropriate") {
    CesiumGltf::Buffer buffer;
    buffer.uri = "data:application/octet-stream;base64,SGVsbG9Xb3JsZCE=";
    buffer.byteLength = static_cast<std::int64_t>(HELLO_WORLD_STR.size());
    buffer.name = "HelloWorldBuffer";
    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF),
    writer.EndObject();

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
    const auto byteLength = static_cast<std::size_t>(firstBuffer["byteLength"].GetInt());
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
    REQUIRE_THROWS_AS(
        CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF),
        CesiumGltf::ByteLengthNotSet
    );
}

TEST_CASE("If writing in GLB mode, buffer[0] automatically has its byteLength calculated based off buffer.cesium.data") {
    CesiumGltf::Buffer buffer;
    buffer.cesium.data = HELLO_WORLD_STR;
    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLB);
    writer.EndObject();

    rapidjson::Document document;
    auto string = writer.toString();
    document.Parse(string.c_str());
    REQUIRE(document.IsObject());
    REQUIRE(document["buffers"].IsArray());

    const auto& buffers = document["buffers"];
    const auto& bufferArray = buffers.GetArray();
    const auto& firstBuffer = bufferArray[0];
    REQUIRE(firstBuffer.IsObject());
    const auto byteLength = static_cast<std::size_t>(firstBuffer["byteLength"].GetInt());
    REQUIRE(!firstBuffer.HasMember("uri"));
    REQUIRE(!firstBuffer.HasMember("name"));
    REQUIRE(byteLength == HELLO_WORLD_STR.size());
}

TEST_CASE("MissingDataSource thrown if ExternalFileURI detected and buffer.cesium.data is empty") {
    CesiumGltf::Buffer buffer;
    buffer.uri = "Foobar.bin";
    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    REQUIRE_THROWS_AS(
        CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF),
        CesiumGltf::MissingDataSource
    );
}

TEST_CASE("extras and extensions are detected and serialized") {
    CesiumGltf::Buffer buffer;
    buffer.extras = CesiumGltf::JsonValue::Object {
        {"some", CesiumGltf::JsonValue("extra")}
    };

    CesiumGltf::JsonValue::Object testExtension {
        {"key", "value"}
    };

    buffer.extensions.emplace_back(testExtension);

    CesiumGltf::JsonWriter writer;
    writer.StartObject();
    CesiumGltf::writeBuffer(std::vector<CesiumGltf::Buffer> { buffer } , writer, CesiumGltf::GLTF);
    writer.EndObject();

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