#pragma once

#include <Cesium3DTiles/Cesium3DTiles.h>
#include <CesiumJsonWriter/ExtensionWriterContext.h>

#include <iostream>
#include <string>

namespace Cesium3DTiles {

class WriterContext {
public:
  using PropertyData = std::vector<const void*>;

  template <typename TObj, typename TExtensionWriter> void registerExtension() {
    this->_context.registerExtension<TObj, TExtensionWriter>();
  }

  std::string writeTileset(const Tileset& tileset) noexcept;
  std::string writePnts(const PntsFeatureTable& pnts) noexcept;

private:
  CesiumJsonWriter::ExtensionWriterContext _context;
};

} // namespace Cesium3DTiles
