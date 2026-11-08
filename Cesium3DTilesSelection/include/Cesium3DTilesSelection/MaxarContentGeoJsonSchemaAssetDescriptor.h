#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <memory>

namespace CesiumAsync {
class AsyncSystem;
}

namespace Cesium3DTilesSelection {

/**
 * @brief A description of a `MAXAR_content_geojson` properties schema that can
 * be loaded from the network.
 *
 * The downloaded document is a Maxar properties schema, not a glTF schema. The
 * load step parses that document and converts it into a @ref CesiumGltf::Schema
 * so the result can be shared with glTF structural metadata.
 */
struct MaxarContentGeoJsonSchemaAssetDescriptor
    : public CesiumAsync::NetworkAssetDescriptor {
  /**
   * @brief Determines if this descriptor is identical to another one.
   */
  bool operator==(
      const MaxarContentGeoJsonSchemaAssetDescriptor& rhs) const noexcept;

  /**
   * @brief Request this asset from the network and return the converted glTF
   * schema.
   *
   * A failed request, a document that is not a Maxar properties schema, or a
   * failed conversion produces an empty result and does not throw.
   *
   * @param asyncSystem The async system.
   * @param pAssetAccessor The asset accessor.
   * @return A future that resolves to the converted schema once the request
   * completes.
   */
  CesiumAsync::Future<CesiumUtility::ResultPointer<CesiumGltf::Schema>> load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const;
};

} // namespace Cesium3DTilesSelection

/** @brief Hash implementation for \ref
 * Cesium3DTilesSelection::MaxarContentGeoJsonSchemaAssetDescriptor. */
template <>
struct std::hash<
    Cesium3DTilesSelection::MaxarContentGeoJsonSchemaAssetDescriptor> {
  /** @brief Returns a `size_t` hash of the provided \ref
   * Cesium3DTilesSelection::MaxarContentGeoJsonSchemaAssetDescriptor. */
  std::size_t operator()(
      const Cesium3DTilesSelection::MaxarContentGeoJsonSchemaAssetDescriptor&
          key) const noexcept;
};
