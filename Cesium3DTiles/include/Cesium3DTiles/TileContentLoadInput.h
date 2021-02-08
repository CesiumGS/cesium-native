#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/BoundingVolume.h"

#include <spdlog/fwd.h>
#include <gsl/span>

#include <memory>

namespace Cesium3DTiles 
{
    /**
     * @brief The information that is passed to a {@link TileContentLoader} to create a {@link TileContentLoadResult}.
     */
    struct CESIUM3DTILES_API TileContentLoadInput {

        TileContentLoadInput(
        	const std::shared_ptr<spdlog::logger> pLogger_,
		    const TileContext& context_,
		    const TileID& tileID_,
		    const BoundingVolume& tileBoundingVolume_,
            double tileGeometricError_,
    		const glm::dmat4& tileTransform_,
		    const std::optional<BoundingVolume>& tileContentBoundingVolume_,
		    TileRefine tileRefine_,
		    const std::string& url_,
            const std::string& contentType_,
		    const gsl::span<const uint8_t>& data_) :
            pLogger(pLogger_),
            context(context_),
            tileID(tileID_),
            tileBoundingVolume(tileBoundingVolume_),
            tileGeometricError(tileGeometricError_),
            tileTransform(tileTransform_),
            tileContentBoundingVolume(tileContentBoundingVolume_),
            tileRefine(tileRefine_),
            url(url_),
            contentType(contentType_),
            data(data_)
        {
        }


        /**
         * @brief The logger that receives details of loading errors and warnings.
         */
		const std::shared_ptr<spdlog::logger> pLogger;

        /**
         * @brief The raw input data
         */
        const gsl::span<const uint8_t>& data;

        // XXX TODO May become optional!
        const std::string contentType;

        /**
         * @brief The source URL
         */
		const std::string& url;

        /**
         * @brief The {@link TileContext}.
         */
		const TileContext& context;

        /**
         * @brief The {@link TileID}
         */
		const TileID& tileID;

        /**
         * @brief The tile {@link BoundingVolume}
         */
		const BoundingVolume& tileBoundingVolume;

        /**
         * @brief Tile content {@link BoundingVolume}
         */
		const std::optional<BoundingVolume>& tileContentBoundingVolume;

        /**
         * @brief The {@link TileRefine}
         */
		const TileRefine tileRefine;

        /**
         * @brief The geometric error
         */
        const double tileGeometricError;

        /**
         * @brief The tile transform
         */
        const glm::dmat4& tileTransform;

    };
}

