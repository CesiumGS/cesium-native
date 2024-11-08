#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <memory>
#include <optional>

namespace CesiumAsync {
class AsyncSystem;
}

namespace CesiumRasterOverlays {

struct LoadedQuadtreeImage {
  CesiumUtility::IntrusivePointer<LoadedRasterOverlayImage> pLoaded = nullptr;
  std::optional<CesiumGeometry::Rectangle> subset = std::nullopt;
};

/**
 * @brief A description of an image that is part of a raster overlay that can be
 * loaded from the network and stored in a `SharedAssetDepot`. It contains the
 * URL, headers, KTX2 transcode targets, and any options specified.
 */
struct NetworkRasterOverlayImageAssetDescriptor
    : public CesiumAsync::NetworkAssetDescriptor {
  /**
   * @brief The supported GPU texture formats used for KTX2 decoding.
   */
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets{};

  /**
   * @brief Options such as the rectangle of this raster overlay image and any
   * credits to attach.
   */
  CesiumRasterOverlays::LoadTileImageFromUrlOptions loadTileOptions;

  /**
   * @brief Determines if this descriptor is identical to another one.
   */
  bool operator==(
      const NetworkRasterOverlayImageAssetDescriptor& rhs) const noexcept;

  /**
   * @brief Request this asset from the network using the provided asset
   * accessor and return the loaded {@link ImageAsset}.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the image asset once the request is
   * complete.
   */
  CesiumAsync::Future<CesiumUtility::ResultPointer<LoadedRasterOverlayImage>>
  load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const;
};

} // namespace CesiumRasterOverlays

template <>
struct std::hash<
    CesiumRasterOverlays::NetworkRasterOverlayImageAssetDescriptor> {
  std::size_t operator()(
      const CesiumRasterOverlays::NetworkRasterOverlayImageAssetDescriptor& key)
      const noexcept;
};

template <>
struct std::hash<CesiumRasterOverlays::LoadTileImageFromUrlOptions> {
  std::size_t
  operator()(const CesiumRasterOverlays::LoadTileImageFromUrlOptions& key)
      const noexcept;
};
