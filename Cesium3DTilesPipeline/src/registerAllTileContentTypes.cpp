#include "Cesium3DTilesPipeline/registerAllTileContentTypes.h"
#include "Batched3DModelContent.h"
#include "Cesium3DTilesPipeline/ExternalTilesetContent.h"
#include "Cesium3DTilesPipeline/GltfContent.h"
#include "Cesium3DTilesPipeline/TileContentFactory.h"
#include "CompositeContent.h"
#include "QuantizedMeshContent.h"

namespace Cesium3DTilesPipeline {

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

  TileContentFactory::registerContentType(
      QuantizedMeshContent::CONTENT_TYPE,
      std::make_shared<QuantizedMeshContent>());
}

} // namespace Cesium3DTilesPipeline
