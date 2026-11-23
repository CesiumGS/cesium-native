#include "CesiumTilesetJson/ConditionalContent.h"
#include "TilesetJsonWriter.h"

#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumTilesetJson/Buffer.h>
#include <CesiumTilesetJsonReader/ConditionalContentReader.h>
#include <CesiumTilesetJsonWriter/ConditionalContentWriter.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace {
void check(const std::string& input, const std::string& expectedOutput) {
  CesiumTilesetJsonReader::ConditionalContentReader reader;
  auto readResult = reader.readFromJson(std::span(
      reinterpret_cast<const std::byte*>(input.c_str()),
      input.size()));
  REQUIRE(readResult.errors.empty());
  REQUIRE(readResult.warnings.empty());
  REQUIRE(readResult.value.has_value());

  CesiumTilesetJson::ConditionalContent& conditionalContent =
      readResult.value.value();

  CesiumTilesetJsonWriter::ConditionalContentWriter writer;
  CesiumTilesetJsonWriter::ConditionalContentWriterResult writeResult =
      writer.writeConditionalContent(conditionalContent);
  const std::vector<std::byte>& conditionalContentBytes =
      writeResult.conditionalContentBytes;

  REQUIRE(writeResult.errors.empty());
  REQUIRE(writeResult.warnings.empty());

  std::string conditionalContentString(
      reinterpret_cast<const char*>(conditionalContentBytes.data()),
      conditionalContentBytes.size());

  rapidjson::Document conditionalContentJson;
  conditionalContentJson.Parse(conditionalContentString.c_str());

  rapidjson::Document expectedJson;
  expectedJson.Parse(expectedOutput.c_str());

  REQUIRE(conditionalContentJson == expectedJson);
}

bool hasSpaces(const std::string& input) {
  return std::count_if(input.begin(), input.end(), [](unsigned char c) {
    return std::isspace(c);
  });
}

} // namespace

TEST_CASE("Writes conditional content JSON") {
  std::string string = R"(
    {
      "conditionalContents" : [ {
        "uri" : "content-0-0-2025-09-25-revision0.glb",
        "keys" : {
          "exampleTimeStamp" : "2025-09-25",
          "exampleRevision" : "revision0"
        }
      }, {
        "uri" : "content-0-0-2025-09-26-revision1.glb",
        "keys" : {
          "exampleTimeStamp" : "2025-09-26",
          "exampleRevision" : "revision1"
        }
      } ]
    }
)";

  check(string, string);
}

TEST_CASE("Writes conditional content JSON with prettyPrint") {
  CesiumTilesetJson::ConditionalContent conditionalContent;
  conditionalContent.conditionalContents.emplace_back().keys.emplace(
      "exampleTimeStamp",
      "2025-09-25");

  CesiumTilesetJsonWriter::ConditionalContentWriter writer;
  CesiumTilesetJsonWriter::ConditionalContentWriterOptions options;
  options.prettyPrint = false;

  CesiumTilesetJsonWriter::ConditionalContentWriterResult writeResult =
      writer.writeConditionalContent(conditionalContent, options);
  const std::vector<std::byte>& conditionalContentBytesCompact =
      writeResult.conditionalContentBytes;

  std::string conditionalContentStringCompact(
      reinterpret_cast<const char*>(conditionalContentBytesCompact.data()),
      conditionalContentBytesCompact.size());

  REQUIRE_FALSE(hasSpaces(conditionalContentStringCompact));

  options.prettyPrint = true;
  writeResult = writer.writeConditionalContent(conditionalContent, options);
  const std::vector<std::byte>& conditionalContentBytesPretty =
      writeResult.conditionalContentBytes;
  std::string conditionalContentStringPretty(
      reinterpret_cast<const char*>(conditionalContentBytesPretty.data()),
      conditionalContentBytesPretty.size());

  REQUIRE(hasSpaces(conditionalContentStringPretty));
}
