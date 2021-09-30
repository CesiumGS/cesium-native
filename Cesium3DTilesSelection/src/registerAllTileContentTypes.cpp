#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"

#include "Batched3DModelContent.h"
#include "Cesium3DTilesSelection/ExternalTilesetContent.h"
#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/TileContentFactory.h"
#include "CompositeContent.h"
#include "PointCloudContent.h"
#include "QuantizedMeshContent.h"

namespace Cesium3DTilesSelection {

void registerAllTileContentTypes() {

  TileContentFactory::registerMagic("glTF", std::make_shared<GltfContent>());
  TileContentFactory::registerMagic(
      "b3dm",
      std::make_shared<Batched3DModelContent>());
  TileContentFactory::registerMagic(
      "cmpt",
      std::make_shared<CompositeContent>());
  TileContentFactory::registerMagic(
      "json",
      std::make_shared<ExternalTilesetContent>());
  TileContentFactory::registerMagic(
      "pnts",
      std::make_shared<PointCloudContent>());

  TileContentFactory::registerContentType(
      QuantizedMeshContent::CONTENT_TYPE,
      std::make_shared<QuantizedMeshContent>());
}

} // namespace Cesium3DTilesSelection
