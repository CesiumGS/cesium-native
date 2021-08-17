#include "Cesium3DTiles/WriterContext.h"
#include "Cesium3DTilesWriter.h"

#include <CesiumJsonWriter/JsonWriter.h>

using namespace Cesium3DTiles;
using namespace CesiumJsonWriter;

void WriterContext::writeTileset(std::ostream& os, const Tileset& tileset) {
  JsonWriter writer;
  TilesetWriter::write(tileset, writer, this->_context);
  os << writer.toString();
}

void WriterContext::writePnts(std::ostream& os, const PntsFeatureTable& pnts) {
  JsonWriter writer;
  PntsFeatureTableWriter::write(pnts, writer, this->_context);
  os << writer.toString();
}
