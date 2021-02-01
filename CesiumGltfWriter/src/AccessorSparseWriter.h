#pragma once
#include "CesiumGltf/AccessorSparse.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace CesiumGltf {
    void writeAccessorSparse(
        const AccessorSparse& accessorSparse,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter);
}