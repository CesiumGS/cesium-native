#pragma once

#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
#include "TilesetLoadPipeline.h"

#include <CesiumAsync/Pipeline.h>

namespace Cesium3DTilesSelection {

class IonAssetPipeline : public TilesetLoadPipeline<IonAssetPipeline> {
public:
  IonAssetPipeline(Tileset& tileset);

  CesiumAsync::Future<ResultOrFailure> begin() {
    return this->getAsyncSystem().createResolvedFuture(
        ResultOrFailure(std::monostate()));
  }

  CesiumAsync::Future<void> onSuccess(std::monostate&&) {
    return this->getAsyncSystem().createResolvedFuture();
  }

  CesiumAsync::Future<void> onFailure(TilesetLoadFailureDetails&&) {
    return this->getAsyncSystem().createResolvedFuture();
  }
};

} // namespace Cesium3DTilesSelection
