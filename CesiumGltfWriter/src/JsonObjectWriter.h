#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/ExtensibleObject.h>
#include <CesiumGltf/JsonValue.h>

namespace CesiumGltf {
void writeJsonValue(
    const JsonValue& value,
    CesiumGltf::JsonWriter& writer,
    bool hasRootObject = true);
}