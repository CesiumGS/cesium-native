#include "Cesium3DTilesWriter/VoxelContentWriter.h"

#include "TilesetJsonWriter.h"
#include "registerExtensions.h"

#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumUtility/Tracing.h>

namespace Cesium3DTilesWriter {

VoxelContentWriter::VoxelContentWriter() { registerExtensions(this->_context); }

CesiumJsonWriter::ExtensionWriterContext& VoxelContentWriter::getExtensions() {
  return this->_context;
}

const CesiumJsonWriter::ExtensionWriterContext&
VoxelContentWriter::getExtensions() const {
  return this->_context;
}

VoxelContentWriterResult VoxelContentWriter::writeVoxelContent(
    const Cesium3DTiles::VoxelContent& voxel,
    const VoxelContentWriterOptions& options) const {
  CESIUM_TRACE("VoxelContentWriter::writeVoxelContent");

  const CesiumJsonWriter::ExtensionWriterContext& context =
      this->getExtensions();

  VoxelContentWriterResult result;
  std::unique_ptr<CesiumJsonWriter::JsonWriter> writer;

  if (options.prettyPrint) {
    writer = std::make_unique<CesiumJsonWriter::PrettyJsonWriter>();
  } else {
    writer = std::make_unique<CesiumJsonWriter::JsonWriter>();
  }

  VoxelContentJsonWriter::write(voxel, *writer, context);
  result.voxelContentBytes = writer->toBytes();

  return result;
}
} // namespace Cesium3DTilesWriter
