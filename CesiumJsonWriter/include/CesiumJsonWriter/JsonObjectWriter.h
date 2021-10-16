#pragma once

#include "JsonWriter.h"

#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonWriter {
void writeJsonValue(
    const CesiumUtility::JsonValue& value,
    CesiumJsonWriter::JsonWriter& writer);
}
