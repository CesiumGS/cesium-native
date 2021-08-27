#include "Cesium3DTiles/WriterContext.h"
#include "Cesium3DTilesWriter.h"

#include <CesiumJsonWriter/JsonWriter.h>

using namespace Cesium3DTiles;
using namespace CesiumJsonWriter;

std::string WriterContext::writeTileset(const Tileset& tileset) noexcept {
  JsonWriter writer;
  TilesetWriter::write(tileset, writer, this->_context);
  return writer.toString();
}

std::string WriterContext::writePnts(const PntsFeatureTable& pnts) noexcept {
  JsonWriter writer;
  PntsFeatureTableWriter::write(pnts, writer, this->_context);
  return writer.toString();
}
