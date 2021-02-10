#pragma once
#include <CesiumGltf/JsonValue.h>
#include <CesiumGltf/ExtensibleObject.h>
#include "JsonWriter.h"

namespace CesiumGltf {
    void writeJsonObject(const JsonValue::Object& object, CesiumGltf::JsonWriter& writer);
}