#pragma once

#include "Cesium3DTiles/Library.h"
#include <functional>

namespace Cesium3DTiles {
    class IAssetAccessor;
    class IPrepareRendererResources;

    class ITaskProcessor {
    public:
        virtual ~ITaskProcessor() = default;
        virtual void startTask(std::function<void()> f) = 0;
    };

    class TilesetExternals {
    public:
        IAssetAccessor* pAssetAccessor;
        IPrepareRendererResources* pPrepareRendererResources;
        ITaskProcessor* pTaskProcessor;
    };

}
