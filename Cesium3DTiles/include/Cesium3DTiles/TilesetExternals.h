#pragma once

#include "Cesium3DTiles/Library.h"
#include <functional>

namespace Cesium3DTiles {
    class IAssetAccessor;
    class IPrepareRendererResources;

    /**
     * @brief When implemented by a rendering engine, allows a {@link Tileset} to create tasks to be asynchronously executed in background threads.
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
         * @brief Starts a task that executes the given function in a background thread.
         * 
         * @param f The function to execute
         */
        virtual void startTask(std::function<void()> f) = 0;
    };

    /**
     * @brief External interfaces used by a {@link Tileset}.
     * 
     * Not supposed to be used by clients.
     */
    class TilesetExternals {
    public:

        /**
         * @brief An external {@link IAssetAccessor}.
         */
        std::shared_ptr<IAssetAccessor> pAssetAccessor;

        /**
         * @brief An external {@link IPrepareRendererResources}.
         */
        std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources;

        /**
         * @brief An external {@link ITaskProcessor}
         */
        std::shared_ptr<ITaskProcessor> pTaskProcessor;
    };

}
