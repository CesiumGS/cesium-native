#pragma once

#include <CesiumI3S/Building.h>
#include <CesiumI3S/PointCloud.h>
#include <CesiumI3S/Scene.h>
#include <CesiumI3SReader/Library.h>
#include <CesiumJsonReader/JsonReader.h>

#include <cstddef>
#include <span>

namespace CesiumI3SReader {

/**
 * @brief Reads an I3S scene layer from JSON bytes into a strongly-typed i3s
 * spec.
 */
class CESIUMI3SREADER_API SceneLayerReader {
public:
  /** @brief Constructs a new SceneLayerReader. */
  SceneLayerReader() noexcept;

  /**
   * @brief Reads a CMN or PSL scene layer (3DObject, IntegratedMesh, or Point).
   *
   * @param data The raw JSON bytes of the layer document.
   * @returns The parsed @ref CesiumI3S::Layer, along with any errors or
   * warnings encountered during parsing.
   */
  CesiumJsonReader::ReadJsonResult<CesiumI3S::Layer>
  readLayer(const std::span<const std::byte>& data) const;

  /**
   * @brief Reads a Building Scene Layer.
   *
   * @param data The raw JSON bytes of the layer document.
   * @returns The parsed @ref CesiumI3S::BuildingLayer, along with any errors or
   * warnings encountered during parsing.
   */
  CesiumJsonReader::ReadJsonResult<CesiumI3S::BuildingLayer>
  readBuildingLayer(const std::span<const std::byte>& data) const;

  /**
   * @brief Reads a Point Cloud Scene Layer.
   *
   * @param data The raw JSON bytes of the layer document.
   * @returns The parsed @ref CesiumI3S::PointCloudLayer, along with any errors
   * or warnings encountered during parsing.
   */
  CesiumJsonReader::ReadJsonResult<CesiumI3S::PointCloudLayer>
  readPointCloudLayer(const std::span<const std::byte>& data) const;
};

} // namespace CesiumI3SReader
