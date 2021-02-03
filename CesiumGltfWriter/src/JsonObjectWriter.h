#pragma once
#include <CesiumGltf/JsonValue.h>
#include <CesiumGltf/ExtensibleObject.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace CesiumGltf {
    void writeJsonObject(const JsonValue::Object& object, rapidjson::Writer<rapidjson::StringBuffer>& writer);
}