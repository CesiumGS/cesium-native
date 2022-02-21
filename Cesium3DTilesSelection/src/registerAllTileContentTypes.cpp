#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"

#include "Batched3DModelContent.h"
#include "Cesium3DTilesSelection/ExternalTilesetContent.h"
#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/TileContentFactory.h"
#include "CompositeContent.h"
#include "QuantizedMeshContent.h"

namespace Cesium3DTilesSelection {

void registerAllTileContentTypes() {

  std::shared_ptr<GltfContent> gltfLoader = std::make_shared<GltfContent>();
  std::shared_ptr<QuantizedMeshContent> quantizedMeshLoader =
      std::make_shared<QuantizedMeshContent>();

  TileContentFactory::registerMagic("glTF", gltfLoader);
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
      quantizedMeshLoader);

  TileContentFactory::registerFileExtension(".gltf", gltfLoader);
  TileContentFactory::registerFileExtension(".glb", gltfLoader);
  TileContentFactory::registerFileExtension(".terrain", quantizedMeshLoader);
}

} // namespace Cesium3DTilesSelection
