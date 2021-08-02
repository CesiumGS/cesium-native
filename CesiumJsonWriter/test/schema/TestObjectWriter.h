#pragma once

#include "TestObject.h"
#include <CesiumJsonWriter/ExtensionWriterContext.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace Test {

struct PropertyWriter {
  using ValueType = Property;

  static void write(
      const Property& obj,
      CesiumJsonWriter::JsonWriter& jsonWriter,
      const CesiumJsonWriter::ExtensionWriterContext& context);
};

struct TestExtensionWriter {
  using ValueType = TestExtension;

  static inline constexpr const char* ExtensionName = "TEST_EXTENSION";

  static void write(
      const TestExtension& obj,
      CesiumJsonWriter::JsonWriter& jsonWriter,
      const CesiumJsonWriter::ExtensionWriterContext& context);
};

struct TestObjectWriter {
  using ValueType = TestObject;

  static void write(
      const TestObject& obj,
      CesiumJsonWriter::JsonWriter& jsonWriter,
      const CesiumJsonWriter::ExtensionWriterContext& context);
};

} // namespace Test
