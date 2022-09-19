#pragma once


#include "CesiumJsonWriter/JsonObjectWriter.h"
#include "CesiumJsonWriter/JsonWriter.h"


// forward declarations
namespace CesiumUtility {
class JsonValue;
}

namespace CesiumJsonWriter {
void writeJsonValue(const CesiumUtility::JsonValue& value, JsonWriter& writer);
}
