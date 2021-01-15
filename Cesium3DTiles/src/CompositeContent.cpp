#include "CompositeContent.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include <rapidjson/document.h>
#include <stdexcept>

namespace {
    #pragma pack(push, 1)
    struct CmptHeader {
        char magic[4];
        uint32_t version;
        uint32_t byteLength;
        uint32_t tilesLength;
    };

    struct InnerHeader {
        char magic[4];
        uint32_t version;
        uint32_t byteLength;
    };
    #pragma pack(pop)

    static_assert(sizeof(CmptHeader) == 16);
    static_assert(sizeof(InnerHeader) == 12);
}

namespace Cesium3DTiles {

    std::unique_ptr<TileContentLoadResult> CompositeContent::load(
		std::shared_ptr<spdlog::logger> pLogger,
		const TileContext& context,
		const TileID& tileID,
		const BoundingVolume& tileBoundingVolume,
		double tileGeometricError,
		const glm::dmat4& tileTransform,
		const std::optional<BoundingVolume>& tileContentBoundingVolume,
		TileRefine tileRefine,
		const std::string& url,
		const gsl::span<const uint8_t>& data
	) {
        if (data.size() < sizeof(CmptHeader)) {
            SPDLOG_LOGGER_WARN(pLogger, "Composite tile {} must be at least 16 bytes.", url);
            return nullptr;
        }

        const CmptHeader* pHeader = reinterpret_cast<const CmptHeader*>(data.data());
        if (std::string(pHeader->magic, 4) != "cmpt") {
            SPDLOG_LOGGER_WARN(pLogger, "Composite tile does not have the expected magic vaue 'cmpt'.");
            return nullptr;
        }

        if (pHeader->version != 1) {
            SPDLOG_LOGGER_WARN(pLogger, "Unsupported composite tile version {}.", pHeader->version);
            return nullptr;
        }

        if (pHeader->byteLength > data.size()) {
            SPDLOG_LOGGER_WARN(pLogger, "Composite tile byteLength is {} but only {} bytes are available.", pHeader->byteLength, data.size());
            return nullptr;
        }

        std::vector<std::unique_ptr<TileContentLoadResult>> innerTiles;

        uint32_t pos = sizeof(CmptHeader);

        for (uint32_t i = 0; i < pHeader->tilesLength; ++i) {
            if (pos + sizeof(InnerHeader) > pHeader->byteLength) {
                SPDLOG_LOGGER_WARN(pLogger, "Composite tile ends before all embedded tiles could be read.");
                break;
            }

            const InnerHeader* pInner = reinterpret_cast<const InnerHeader*>(data.data() + pos);
            if (pos + pInner->byteLength > pHeader->byteLength) {
                SPDLOG_LOGGER_WARN(pLogger, "Composite tile ends before all embedded tiles could be read.");
                break;
            }

            gsl::span<const uint8_t> innerData(data.data() + pos, pInner->byteLength);

            std::unique_ptr<TileContentLoadResult> pInnerLoadResult = TileContentFactory::createContent(
                pLogger,
                context,
                tileID,
                tileBoundingVolume,
                tileGeometricError,
                tileTransform,
                tileContentBoundingVolume,
                tileRefine,
                url,
                "",
                innerData
            );

            if (pInnerLoadResult) {
                innerTiles.emplace_back(std::move(pInnerLoadResult));
            }
            
            pos += pInner->byteLength;
        }

        if (innerTiles.size() == 0) {
            if (pHeader->tilesLength > 0) {
                SPDLOG_LOGGER_WARN(pLogger, "Composite tile does not contain any loadable inner tiles.");
            }
            return nullptr;
        } else if (innerTiles.size() == 1) {
            return std::move(innerTiles[0]);
        } else {
            // TODO: combine all inner tiles into one glTF instead of return only the first.
            SPDLOG_LOGGER_WARN(pLogger, "Composite tile contains multiple loadable inner tiles. Due to a temporary limitation, only the first will be used.");
            return std::move(innerTiles[0]);
        }
    }

}