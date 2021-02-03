#include "catch2/catch.hpp"
// TODO: This path should NOT be relative, should be accessible using
//       target_include_directories(...)
#include "../src/JsonObjectWriter.h"
#include <CesiumGltf/JsonValue.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>

using namespace CesiumGltf;
using namespace rapidjson;

using Object = JsonValue::Object;
using Value = JsonValue;
using Array = JsonValue::Array;
using Number = JsonValue::Number;
using Bool = JsonValue::Bool;
using Null = JsonValue::Null;

TEST_CASE("TestExtrasWjriter") {
    SECTION("[{}, {}, {}]") {
        StringBuffer strBuffer;
        Writer<StringBuffer> writer(strBuffer);
        // clang-format off
        const auto extrasObject = Object {
            {"extras",  Array {Object {}, Object{}, Object{}} }
        };
        // clang-format on

        writeJsonObject(extrasObject, writer);
        auto str = std::string(strBuffer.GetString());
        REQUIRE(str == R"({"extras":[{},{},{}]})");
    }

    SECTION(R"("A": {"B": "C"{}})") {
        StringBuffer strBuffer;
        Writer<StringBuffer> writer(strBuffer);
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

        writeJsonObject(extrasObject, writer);
        auto str = std::string(strBuffer.GetString());
        REQUIRE(str == R"({"extras":{"A":{"B":{"C":{}}}}})");
    }

    SECTION(R"([[[1 -2,false,null,true,{"emojis": "😂👽🇵🇷"}]]])") {
        StringBuffer strBuffer;
        Writer<StringBuffer> writer(strBuffer);
        // clang-format off
        const auto extrasObject = Object {{
            "extras", Array {{{
                Number(1), Number(-2), Bool(false), Null(), Bool(true),
                Object {{ "emojis", "😂👽🇵🇷" }}
            }}}
        }};
        // clang-format on

        writeJsonObject(extrasObject, writer);
        auto str = std::string(strBuffer.GetString());
        REQUIRE(
            str ==
            R"({"extras":[[[1.0,-2.0,false,null,true,{"emojis":"😂👽🇵🇷"}]]]})");
    }

    SECTION("Empty object is serialized correctly") {
        StringBuffer strBuffer;
        Writer<StringBuffer> writer(strBuffer);
        writeJsonObject(Object{}, writer);
        REQUIRE(std::string(strBuffer.GetString()) == "{}");
    }
}