#pragma once

#include <Cesium3DTilesSelection/Exp_TileUserDataStorage.h>
#include <CesiumGltf/Model.h>

#include <any>
#include <optional>
#include <variant>

namespace Cesium3DTilesSelection {
class TilesetContentLoader;

enum class TileLoadState {
  Failed = -2,
  FailedTemporarily = -1,
  Unloaded = 0,
  ContentLoading = 1,
  ContentLoaded = 2,
  Done = 3
};

struct TileUnknownContent {};

struct TileEmptyContent {};

struct TileExternalContent {};

struct TileRenderContent {
  std::optional<CesiumGltf::Model> model{};
};

using TileContentKind = std::variant<
    TileUnknownContent,
    TileEmptyContent,
    TileExternalContent,
    TileRenderContent>;

class TileContent {
public:
  TileContent(TilesetContentLoader* pLoader);

  TileLoadState getState() const noexcept;

  uint16_t getHttpStatusCode() const noexcept;

  bool isExternalContent() const noexcept;

  bool isEmptyContent() const noexcept;

  bool isRenderContent() const noexcept;

  const TileRenderContent* getRenderContent() const noexcept;

  TileRenderContent* getRenderContent() noexcept;

  TilesetContentLoader* getLoader() noexcept;

private:
  void setContentKind(TileContentKind&& contentKind);

  void setContentKind(const TileContentKind& contentKind);

  void setState(TileLoadState state) noexcept;

  void setHttpStatusCode(uint16_t statusCode) noexcept;

  TileUserDataStorage::Handle getLoaderCustomDataHandle() const;

  void setLoaderCustomDataHandle(TileUserDataStorage::Handle handle);

  uint16_t _httpStatusCode;
  TileLoadState _state;
  TileContentKind _contentKind;
  TileUserDataStorage::Handle _loaderCustomDataHandle;
  TilesetContentLoader* _pLoader;

  friend class TilesetContentLoader;
};
} // namespace Cesium3DTilesSelection
