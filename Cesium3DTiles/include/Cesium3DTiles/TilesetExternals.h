#pragma once

#include "Cesium3DTiles/Library.h"
#include <functional>

namespace Cesium3DTiles {
    class IAssetAccessor;
    class IPrepareRendererResources;

    /**
     * @brief Interface for classes that can process tasks.
     *
     * Not supposed to be used by clients.
     */
    class ITaskProcessor {
    public:

        /**
         * @brief Default destructor
         */
        virtual ~ITaskProcessor() = default;

        /**
         * @brief Start a task that executes the given function
         * 
         * @param f The function to execute
         */
        virtual void startTask(std::function<void()> f) = 0;
    };

    /**
     * @brief Infrastructure elements for a {@link Tileset}.
     * 
     * Not supposed to be used by clients.
     */
    class TilesetExternals {
    public:

        /**
         * @brief An external {@link IAssetAccessor}
         */
        IAssetAccessor* pAssetAccessor;

        /**
         * @brief External {@link IPrepareRendererResources}
         */
        IPrepareRendererResources* pPrepareRendererResources;

        /**
         * @brief An external {@link ITaskProcessor}
         */
        ITaskProcessor* pTaskProcessor;
    };

}
