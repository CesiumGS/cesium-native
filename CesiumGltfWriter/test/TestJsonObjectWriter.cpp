#include "JsonObjectWriter.h"
#include "JsonWriter.h"
#include <CesiumUtility/JsonValue.h>
#include <catch2/catch.hpp>
#include <cstdint>
#include <string>

using namespace CesiumUtility;
using namespace rapidjson;

using Object = JsonValue::Object;
using Value = JsonValue;
using Array = JsonValue::Array;
using Bool = JsonValue::Bool;
using Null = JsonValue::Null;

TEST_CASE("TestJsonObjectWriter") {
  SECTION("[{}, {}, {}]") {
    CesiumGltf::JsonWriter writer;
    const auto extrasObject =
        Object{{"extras", Array{Object{}, Object{}, Object{}}}};
    writeJsonValue(extrasObject, writer, false);
    REQUIRE(writer.toStringView() == R"({"extras":[{},{},{}]})");
  }

  SECTION("[0,1,2.5]") {
    CesiumGltf::JsonWriter writer;
    const auto extrasObject =
        Array{std::int64_t(0), std::uint64_t(1), double(2.5)};
    writeJsonValue(extrasObject, writer, false);
    REQUIRE(writer.toStringView() == R"([0,1,2.5])");
  }

  SECTION("[👀]") {
    CesiumGltf::JsonWriter writer;
    writer.StartArray();
    writeJsonValue(JsonValue("👀"), writer, true);
    writer.EndArray();
    REQUIRE(writer.toStringView() == "[\"👀\"]");
  }

  SECTION(R"("A": {"B": "C"{}})") {
    CesiumGltf::JsonWriter writer;
    // clang-format off
        const auto extrasObject = Object {{
            "extras", Object {{
                "A", Object {{
                    "B", Object {{
                        "C", Object {}
                    }}
                }}
            }}
        }};
    // clang-format on

    writeJsonValue(extrasObject, writer, false);
    REQUIRE(writer.toStringView() == R"({"extras":{"A":{"B":{"C":{}}}}})");
  }

  SECTION(R"([[[1 -2,false,null,true,{"emojis": "😂👽🇵🇷"}]]])") {
    CesiumGltf::JsonWriter writer;
    // clang-format off
        const auto extrasObject = Object {{
            "extras", Array {{{
                1.0, -2.0, Bool(false), Null(), Bool(true),
                Object {{ "emojis", "😂👽🇵🇷" }} }}}
        }};
    // clang-format on

    writeJsonValue(extrasObject, writer, false);
    REQUIRE(
        writer.toStringView() ==
        R"({"extras":[[[1.0,-2.0,false,null,true,{"emojis":"😂👽🇵🇷"}]]]})");
  }

  SECTION("Empty object is serialized correctly") {
    CesiumGltf::JsonWriter writer;
    writeJsonValue(Object{}, writer, false);
    REQUIRE(writer.toStringView() == "{}");
  }
}
