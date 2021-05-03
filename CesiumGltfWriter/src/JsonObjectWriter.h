#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/ExtensibleObject.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumGltf {
void writeJsonValue(
    const CesiumUtility::JsonValue& value,
    CesiumGltf::JsonWriter& writer);
}
