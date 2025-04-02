#pragma once

#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorDocument.h>
#include <CesiumVectorData/VectorNode.h>

#include <span>
#include <cstddef>

namespace CesiumVectorData {

CesiumUtility::Result<VectorNode> parseGeoJson(const std::span<std::byte>& bytes);

}