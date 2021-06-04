#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContentLoadInput.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include "Cesium3DTiles/TileContentLoader.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "CesiumAsync/AsyncSystem.h"

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace Cesium3DTiles {
class TileContent;
class Tileset;

/**
 * @brief Creates {@link TileContentLoadResult} objects from a
 * {@link TileContentLoadInput}.
 *
 * The class offers a lookup functionality for registering
 * {@link TileContentLoader} instances that can create
 * {@link TileContentLoadResult} instances from a
 * {@link TileContentLoadInput}.
 *
 * The loaders are registered based on the magic header or the content type
 * of the input data. The raw data (i.e. the `data` of the
 * {@link TileContentLoadInput}) is usually received as a response to a
 * network request, and the first four bytes of the raw data form the magic
 * header.
 * Based on this header or the content type of the network response, the loader
 * that will be used for processing the input can be looked up.
 */
class CESIUM3DTILES_API TileContentFactory final {
public:
  TileContentFactory() = delete;

  /**
   * @brief Register the given function for the given magic header.
   *
   * The given magic header is a 4-character string. It will be compared
   * to the first 4 bytes of the raw input data, to decide whether the
   * given factory function should be used to create the
   * {@link TileContentLoadResult} from the input data.
   *
   * @param magic The string describing the magic header.
   * @param pLoader The loader that will be used to create the tile content.
   */
  static void registerMagic(
      const std::string& magic,
      const std::shared_ptr<TileContentLoader>& pLoader);

  /**
   * @brief Register the given function for the given content type.
   *
   * The given string describes the content type of a network response.
   * It is used for deciding whether the given loader should be
   * used to create the {@link TileContentLoadResult} from a
   * {@link TileContentLoadInput} with the same `contentType`.
   *
   * @param contentType The string describing the content type.
   * @param pLoader The loader that will be used to create the tile content
   */
  static void registerContentType(
      const std::string& contentType,
      const std::shared_ptr<TileContentLoader>& pLoader);

  /**
   * @brief Creates the {@link TileContentLoadResult} from the given
   * {@link TileContentLoadInput}.
   *
   * This will look up the {@link TileContentLoader} that can be used to
   * process the given input data, based on all loaders that
   * have been registered with {@link TileContentFactory::registerMagic}
   * or {@link TileContentFactory::registerContentType}.
   *
   * It will first try to find a loader based on the magic header
   * of the `data` in the given input. If no matching loader is found, then
   * it will look up a loader based on the `contentType` of the given input.
   * (This will ignore any parameters that may appear after a `;` in the
   * `contentType` string).
   *
   * If no such loader is found then `nullptr` is returned.
   *
   * If a matching loader is found, it will be applied to the given
   * input, and the result will be returned.
   *
   * @param input The {@link TileContentLoadInput}.
   * @return The {@link TileContentLoadResult}, or `nullptr` if there is
   * no loader registered for the magic header of the given
   * input, and no loader for the content type of the input.
   */
  static CesiumAsync::Future<std::unique_ptr<TileContentLoadResult>>
  createContent(const CesiumAsync::AsyncSystem& asyncSystem, const TileContentLoadInput& input);

private:
  static std::optional<std::string>
  getMagic(const gsl::span<const std::byte>& data);

  static std::unordered_map<std::string, std::shared_ptr<TileContentLoader>>
      _loadersByMagic;
  static std::unordered_map<std::string, std::shared_ptr<TileContentLoader>>
      _loadersByContentType;
};

} // namespace Cesium3DTiles
