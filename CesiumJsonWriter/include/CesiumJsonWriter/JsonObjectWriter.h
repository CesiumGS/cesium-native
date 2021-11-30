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
void writeJsonValue(const CesiumUtility::JsonValue& value, JsonWriter& writer);
}
