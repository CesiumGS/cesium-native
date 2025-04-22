#pragma once

#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorDocument.h>
#include <CesiumVectorData/VectorNode.h>

#include <rapidjson/document.h>

#include <cstddef>
#include <span>

namespace CesiumVectorData {

CesiumUtility::Result<VectorNode>
parseGeoJson(const std::span<const std::byte>& bytes);

CesiumUtility::Result<VectorNode>
parseGeoJson(const rapidjson::Document& document);

} // namespace CesiumVectorData