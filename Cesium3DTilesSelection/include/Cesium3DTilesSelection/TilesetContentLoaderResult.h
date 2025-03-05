#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumUtility/ErrorList.h>

#include <memory>
#include <type_traits>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief Credit information obtained from a @ref TilesetContentLoader.
 */
struct LoaderCreditResult {
  /**
   * @brief The text of this credit.
   */
  std::string creditText;

  /**
   * @brief Whether the credit must be shown on screen or not.
   */
  bool showOnScreen;
};

/**
 * @brief The result of creating a @ref TilesetContentLoader, including the
 * status, request, and any errors, as well as the root tile, credits, and the
 * loader itself.
 */
template <class TilesetContentLoaderType> struct TilesetContentLoaderResult {
  TilesetContentLoaderResult() = default;

  /**
   * @brief Creates a new @ref TilesetContentLoaderResult.
   *
   * @param pLoader_ The @ref TilesetContentLoader that this result is for.
   * @param pRootTile_ The root @ref Tile created by this @ref
   * TilesetContentLoader.
   * @param credits_ A set of @ref LoaderCreditResult objects representing the
   * credits loaded.
   * @param requestHeaders_ The headers used for this tileset request.
   * @param errors_ Any warnings or errors that arose while creating this @ref
   * TilesetContentLoader.
   */
  TilesetContentLoaderResult(
      std::unique_ptr<TilesetContentLoaderType>&& pLoader_,
      std::unique_ptr<Tile>&& pRootTile_,
      std::vector<LoaderCreditResult>&& credits_,
      std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders_,
      CesiumUtility::ErrorList&& errors_)
      : pLoader{std::move(pLoader_)},
        pRootTile{std::move(pRootTile_)},
        credits{std::move(credits_)},
        requestHeaders{std::move(requestHeaders_)},
        errors{std::move(errors_)} {}

  TilesetContentLoaderResult(const TilesetContentLoaderResult&) = delete;
  /** @brief Move constructor for @ref TilesetContentLoaderResult */
  TilesetContentLoaderResult(TilesetContentLoaderResult&&) noexcept = default;
  TilesetContentLoaderResult&
  operator=(const TilesetContentLoaderResult&) = delete;
  /** @brief Move assignment operator for @ref TilesetContentLoaderResult */
  TilesetContentLoaderResult&
  operator=(TilesetContentLoaderResult&&) noexcept = default;

  /**
   * @brief Move constructor for creating a `TilesetContentLoaderResult<T>` from
   * a `TilesetContentLoaderResult<U>` where `U` can be converted to `T`.
   */
  template <
      class OtherLoaderType,
      typename Enable_Type = std::enable_if_t<
          !std::is_same_v<OtherLoaderType, TilesetContentLoaderResult> &&
              std::is_convertible_v<
                  std::unique_ptr<OtherLoaderType>,
                  std::unique_ptr<TilesetContentLoaderType>>,
          int>>
  TilesetContentLoaderResult(
      TilesetContentLoaderResult<OtherLoaderType>&& rhs) noexcept
      : pLoader{std::move(rhs.pLoader)},
        pRootTile{std::move(rhs.pRootTile)},
        credits{std::move(rhs.credits)},
        requestHeaders{std::move(rhs.requestHeaders)},
        errors{std::move(rhs.errors)} {}

  /**
   * @brief Move assignment operator for creating a
   * `TilesetContentLoaderResult<T>` from a `TilesetContentLoaderResult<U>`
   * where `U` can be converted to `T`.
   */
  template <
      class OtherLoaderType,
      typename Enable_Type = std::enable_if_t<
          !std::is_same_v<OtherLoaderType, TilesetContentLoaderResult> &&
              std::is_convertible_v<
                  std::unique_ptr<OtherLoaderType>,
                  std::unique_ptr<TilesetContentLoaderType>>,
          int>>
  TilesetContentLoaderResult&
  operator=(TilesetContentLoaderResult<OtherLoaderType>&& rhs) noexcept {
    using std::swap;
    swap(this->pLoader, rhs.pLoader);
    swap(this->pRootTile, rhs.pRootTile);
    swap(this->credits, rhs.credits);
    swap(this->requestHeaders, rhs.requestHeaders);
    swap(this->errors, rhs.errors);

    return *this;
  }

  /** @brief The @ref TilesetContentLoader that produced this result. */
  std::unique_ptr<TilesetContentLoaderType> pLoader;
  /** @brief The root @ref Tile object from the created loader. */
  std::unique_ptr<Tile> pRootTile;
  /** @brief A set of @ref LoaderCreditResult objects created by the loader. */
  std::vector<LoaderCreditResult> credits;
  /** @brief The request headers used to fetch the tileset. */
  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
  /** @brief Any errors or warnings that arose while creating this @ref
   * TilesetContentLoader. */
  CesiumUtility::ErrorList errors;
  /** @brief The HTTP status code returned when attempting to create this @ref
   * TilesetContentLoader. */
  uint16_t statusCode{200};
};
} // namespace Cesium3DTilesSelection
