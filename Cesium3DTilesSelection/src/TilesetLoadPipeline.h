#pragma once

#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"

#include <CesiumAsync/Pipeline.h>

namespace Cesium3DTilesSelection {

template <typename TDerived>
class TilesetLoadPipeline
    : public CesiumAsync::
          Pipeline<TDerived, TilesetLoadFailureDetails, std::monostate> {
public:
  TilesetLoadPipeline(Tileset& tileset)
      : _tileset(tileset),
        Pipeline(tileset.getAsyncSystem(), tileset.getExternals().pLogger) {
    ++this->_tileset._loadsInProgress;
  }

  virtual ~TilesetLoadPipeline() {
    this->_tileset.notifyTileDoneLoading(nullptr);
  }

  CesiumAsync::Future<
      std::pair<TilesetLoadFailureDetails, CesiumAsync::FailureAction>>
  handleFailure(TilesetLoadFailureDetails&& failure) {
    return this->getAsyncSystem().createResolvedFuture(
        std::make_pair(std::move(failure), FailureAction::GiveUp));
  }

private:
  Tileset& _tileset;
};

} // namespace Cesium3DTilesSelection
