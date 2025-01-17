#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>

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
  SUBCASE("[{}, {}, {}]") {
    CesiumJsonWriter::JsonWriter writer;
    const auto extrasObject =
        Object{{"extras", Array{Object{}, Object{}, Object{}}}};
    writeJsonValue(extrasObject, writer);
    REQUIRE(writer.toStringView() == R"({"extras":[{},{},{}]})");
  }

  SUBCASE("[0,1,2.5]") {
    CesiumJsonWriter::JsonWriter writer;
    const auto extrasObject =
        Array{std::int64_t(0), std::uint64_t(1), double(2.5)};
    writeJsonValue(extrasObject, writer);
    REQUIRE(writer.toStringView() == R"([0,1,2.5])");
  }

  SUBCASE("[👀]") {
    CesiumJsonWriter::JsonWriter writer;
    writer.StartArray();
    writeJsonValue(JsonValue("👀"), writer);
    writer.EndArray();
    REQUIRE(writer.toStringView() == "[\"👀\"]");
  }

  SUBCASE(R"("A": {"B": "C"{}})") {
    CesiumJsonWriter::JsonWriter writer;
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

    writeJsonValue(extrasObject, writer);
    REQUIRE(writer.toStringView() == R"({"extras":{"A":{"B":{"C":{}}}}})");
  }

  SUBCASE(R"([[[1 -2,false,null,true,{"emojis": "😂👽🇵🇷"}]]])") {
    CesiumJsonWriter::JsonWriter writer;
    // clang-format off
        const auto extrasObject = Object {{
            "extras", Array {{{
                1.0, -2.0, Bool(false), Null(), Bool(true),
                Object {{ "emojis", "😂👽🇵🇷" }} }}}
        }};
    // clang-format on

    writeJsonValue(extrasObject, writer);
    REQUIRE(
        writer.toStringView() ==
        R"({"extras":[[[1.0,-2.0,false,null,true,{"emojis":"😂👽🇵🇷"}]]]})");
  }

  SUBCASE("Empty object is serialized correctly") {
    CesiumJsonWriter::JsonWriter writer;
    writeJsonValue(Object{}, writer);
    REQUIRE(writer.toStringView() == "{}");
  }
}
