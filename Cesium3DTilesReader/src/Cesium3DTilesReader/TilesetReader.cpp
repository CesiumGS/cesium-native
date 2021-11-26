#include "Cesium3DTilesReader/TilesetReader.h"

#include "Cesium3DTilesReader/Extension3dTilesBoundingVolumeS2JsonHandler.h"
#include "Cesium3DTilesReader/Extension3dTilesContentGltfJsonHandler.h"
#include "Cesium3DTilesReader/Extension3dTilesImplicitTilingJsonHandler.h"
#include "Cesium3DTilesReader/TilesetJsonHandler.h"

#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesReader {

namespace {

TilesetReaderResult readTilesetJson(
    const CesiumJsonReader::ExtensionReaderContext& context,
    const gsl::span<const std::byte>& data) {

  CESIUM_TRACE("Cesium3DTilesReader::TilesetReader::readTilesetJson");

  TilesetJsonHandler tilesetHandler(context);
  CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Tileset> jsonResult =
      CesiumJsonReader::JsonReader::readJson(data, tilesetHandler);

  return TilesetReaderResult{
      std::move(jsonResult.value),
      std::move(jsonResult.errors),
      std::move(jsonResult.warnings)};
}

} // namespace

TilesetReader::TilesetReader() {
  this->_context.registerExtension<
      Cesium3DTiles::Tileset,
      Extension3dTilesContentGltfJsonHandler>();

  this->_context.registerExtension<
      Cesium3DTiles::Tile,
      Extension3dTilesImplicitTilingJsonHandler>();

  this->_context.registerExtension<
      Cesium3DTiles::BoundingVolume,
      Extension3dTilesBoundingVolumeS2JsonHandler>();
}

CesiumJsonReader::ExtensionReaderContext& TilesetReader::getExtensions() {
  return this->_context;
}

const CesiumJsonReader::ExtensionReaderContext&
TilesetReader::getExtensions() const {
  return this->_context;
}

TilesetReaderResult TilesetReader::readTileset(
    const gsl::span<const std::byte>& data,
    const ReadTilesetOptions& /*options*/) const {
  const CesiumJsonReader::ExtensionReaderContext& context =
      this->getExtensions();
  TilesetReaderResult result = readTilesetJson(context, data);

  return result;
}
} // namespace Cesium3DTilesReader