#pragma once

#include <Cesium3DTilesSelection/ErrorList.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>

#include <memory>
#include <type_traits>
#include <vector>

namespace Cesium3DTilesSelection {
struct LoaderCreditResult {
  std::string creditText;

  bool showOnScreen;
};

template <class TilesetContentLoaderType> struct TilesetContentLoaderResult {
  TilesetContentLoaderResult() = default;

  TilesetContentLoaderResult(
      std::unique_ptr<TilesetContentLoaderType>&& pLoader_,
      std::unique_ptr<Tile>&& pRootTile_,
      std::vector<LoaderCreditResult>&& credits_,
      std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders_,
      ErrorList&& errors_)
      : pLoader{std::move(pLoader_)},
        pRootTile{std::move(pRootTile_)},
        credits{std::move(credits_)},
        requestHeaders{std::move(requestHeaders_)},
        errors{std::move(errors_)} {}

  TilesetContentLoaderResult(const TilesetContentLoaderResult&) = delete;

  TilesetContentLoaderResult(TilesetContentLoaderResult&&) noexcept = default;

  TilesetContentLoaderResult&
  operator=(const TilesetContentLoaderResult&) = delete;

  TilesetContentLoaderResult&
  operator=(TilesetContentLoaderResult&&) noexcept = default;

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

  std::unique_ptr<TilesetContentLoaderType> pLoader;

  std::unique_ptr<Tile> pRootTile;

  std::vector<LoaderCreditResult> credits;

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;

  ErrorList errors;

  uint16_t statusCode{200};
};
} // namespace Cesium3DTilesSelection
