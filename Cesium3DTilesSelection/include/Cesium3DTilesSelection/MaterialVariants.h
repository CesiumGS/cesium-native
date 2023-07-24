#pragma once

#include "Library.h"

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 */
struct CESIUM3DTILESSELECTION_API MaterialVariants {
public:
  std::vector<std::string> tilesetMaterialVariantNames;
  std::vector<std::vector<std::string>> groupsMaterialVariantNames;
};

} // namespace Cesium3DTilesSelection
