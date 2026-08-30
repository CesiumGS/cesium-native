#pragma once
#include <CesiumI3S/Library.h>

namespace CesiumI3S {

/** @brief Operations that may be supported by any I3S scene layer
 * (3DSceneLayer.cmn.md, layer.bld.md, layer.pcsl.md). */
enum class LayerCapabilities { View, Query, Edit, Extract };

} // namespace CesiumI3S
