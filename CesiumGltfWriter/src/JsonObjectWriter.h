#pragma once
#include <CesiumGltf/JsonValue.h>
#include <CesiumGltf/ExtensibleObject.h>
#include "JsonWriter.h"

namespace CesiumGltf {
    void writeJsonValue(const JsonValue& value, CesiumGltf::JsonWriter& writer);
}