#pragma once

#include "Cesium3DTiles/TileContentLoadInput.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "CesiumAsync/AsyncSystem.h"

namespace Cesium3DTiles {
/**
 * @brief Can create a {@link TileContentLoadResult} from a {@link
 * TileContentLoadInput}.
 */
class CESIUM3DTILES_API TileContentLoader {

public:
  virtual ~TileContentLoader() = default;

  /**
   * @brief Loads the tile content from the given input.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param tile The tile for which content is to be loaded. This tile instance
   * is guaranteed to remain valid until the returned Future resolves or
   * rejects, so it is safe to use a pointer or reference to it throughout the
   * entire load process.
   * @param url The URL from which the content was loaded.
   * @param contentType The content type. If the data was obtained via a HTTP
   * response, then this will be the `Content-Type` header of that response. The
   * {@link TileContentFactory} will try to interpret the data based on this
   * content type. If the data was not directly obtained from an HTTP response,
   * then this may be the empty string.
   * @return A Future that resolves to the loaded content. This may be the
   * `nullptr` if the tile content could not be loaded.
   */
  virtual CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const Tile& tile,
      const std::string& url,
      const std::string& contentType,
      const gsl::span<const std::byte>& data) = 0;
};
} // namespace Cesium3DTiles
