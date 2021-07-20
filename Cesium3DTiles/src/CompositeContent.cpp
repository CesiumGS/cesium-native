#include "CompositeContent.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumUtility/Tracing.h"
#include <cstddef>
#include <rapidjson/document.h>
#include <stdexcept>
#include <functional>
#include <thread>
#include <chrono>

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
} // namespace

namespace Cesium3DTiles {

namespace {
/**
 * @brief Derive a {@link TileContentLoadInput} from the given one.
 *
 * This will return a new instance where all properies are set to be
 * the same as in the given input, except for the content type (which
 * is set to be the empty string), and the data, which is set to be
 * the given derived data.
 *
 * @param input The original input.
 * @param derivedData The data for the result.
 * @return The result.
 */
TileContentLoadInput derive(
    const TileContentLoadInput& input,
    const gsl::span<const std::byte>& derivedData) {
  return TileContentLoadInput(
      input.pLogger,
      derivedData,
      "",
      input.url,
      input.tileID,
      input.tileBoundingVolume,
      input.tileContentBoundingVolume,
      input.tileRefine,
      input.tileGeometricError,
      input.tileTransform,
      input.contentOptions);
}
} // namespace

CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
CompositeContent::load(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input) {
  CESIUM_TRACE("Cesium3DTiles::CompositeContent::load");
  const std::shared_ptr<spdlog::logger>& pLogger = input.pLogger;
  const gsl::span<const std::byte>& data = input.data;
  const std::string& url = input.url;

  if (data.size() < sizeof(CmptHeader)) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Composite tile {} must be at least 16 bytes.",
        url);
    return asyncSystem.createResolvedFuture(std::unique_ptr<TileContentLoadResult>(nullptr));
  }

  const CmptHeader* pHeader = reinterpret_cast<const CmptHeader*>(data.data());
  if (std::string(pHeader->magic, 4) != "cmpt") {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Composite tile does not have the expected magic vaue 'cmpt'.");
    return asyncSystem.createResolvedFuture(std::unique_ptr<TileContentLoadResult>(nullptr));
  }

  if (pHeader->version != 1) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Unsupported composite tile version {}.",
        pHeader->version);
    return asyncSystem.createResolvedFuture(std::unique_ptr<TileContentLoadResult>(nullptr));
  }

  if (pHeader->byteLength > data.size()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Composite tile byteLength is {} but only {} bytes are available.",
        pHeader->byteLength,
        data.size());
    return asyncSystem.createResolvedFuture(std::unique_ptr<TileContentLoadResult>(nullptr));
  }

  std::vector<CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>> innerTiles(pHeader->tilesLength);
  uint32_t pos = sizeof(CmptHeader);

  for (uint32_t i = 0; i < pHeader->tilesLength && pos < pHeader->byteLength; ++i) {
    if (pos + sizeof(InnerHeader) > pHeader->byteLength) {
      SPDLOG_LOGGER_WARN(
          pLogger,
          "Composite tile ends before all embedded tiles could be read.");
      pos = static_cast<uint32_t>(pHeader->byteLength);
      break;
    }

    const InnerHeader* pInner =
        reinterpret_cast<const InnerHeader*>(data.data() + pos);
    if (pos + pInner->byteLength > pHeader->byteLength) {
      SPDLOG_LOGGER_WARN(
          pLogger,
          "Composite tile ends before all embedded tiles could be read.");
      pos = static_cast<uint32_t>(pHeader->byteLength);
      break;
    }

    gsl::span<const std::byte> innerData(data.data() + pos, pInner->byteLength);

    pos += pInner->byteLength;

    innerTiles.push_back( 
        TileContentFactory::createContent(
          asyncSystem, 
          derive(input, innerData)));
  }

  return asyncSystem
    // TODO: How dubious is this exactly?
    .runInWorkerThread(
      [&unresolvedFutures]() -> int {
        while (unresolvedFutures > 0) {
          std::this_thread::yield();//(std::chrono::milliseconds(10));
        }
        return 0;
      })
    .thenInMainThread(
      [&pLogger,
       &innerTiles,
       tilesLength = pHeader->tilesLength](int) mutable -> std::unique_ptr<TileContentLoadResult> {
        
        if (innerTiles.empty()) {
          if (tilesLength > 0) {
            SPDLOG_LOGGER_WARN(
                pLogger,
                "Composite tile does not contain any loadable inner tiles.");
          }
          return std::make_unique<TileContentLoadResult>();
        }
        if (innerTiles.size() == 1) {
          return std::move(innerTiles[0]);
        }

        std::unique_ptr<TileContentLoadResult> pResult = std::move(innerTiles[0]);

        for (size_t i = 1; i < innerTiles.size(); ++i) {
          if (innerTiles[i]->model) {
            continue;
          }

          if (pResult->model) {
            pResult->model.value().merge(std::move(innerTiles[i]->model.value()));
          } else {
            pResult->model = std::move(innerTiles[i]->model);
          }
        }

        return std::move(pResult);
      });
}

} // namespace Cesium3DTiles