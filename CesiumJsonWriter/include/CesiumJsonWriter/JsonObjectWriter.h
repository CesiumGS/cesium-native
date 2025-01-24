#pragma once

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

// forward declarations
namespace CesiumUtility {
class JsonValue;
}

namespace CesiumJsonWriter {
/**
 * @brief Writes the given \ref CesiumUtility::JsonValue to the provided writer.
 *
 * @param value The value to write.
 * @param writer The writer used to write the provided value.
 */
void writeJsonValue(const CesiumUtility::JsonValue& value, JsonWriter& writer);
} // namespace CesiumJsonWriter
