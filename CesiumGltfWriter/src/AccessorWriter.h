#pragma once
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/AccessorSpec.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace CesiumGltf {
    void writeAccessor(
        const std::vector<Accessor>& accessors,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter);
}