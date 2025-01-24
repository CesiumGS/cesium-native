#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/Model.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumUtility/CreditSystem.h>

#include <memory>
#include <variant>
#include <vector>

namespace Cesium3DTilesSelection {
/**
 * @brief A content tag that indicates the {@link TilesetContentLoader} does not
 * know if a tile's content will point to a mesh content or an external
 * tileset. The content of the tile is only known when the loader loads the tile
 * to inspect the content.
 */
struct CESIUM3DTILESSELECTION_API TileUnknownContent {};

/**
 * @brief A content tag that indicates a tile has no content.
 *
 * There are two possible ways to handle a tile with no content:
 *
 * 1. Treat it as a placeholder used for more efficient culling, but
 *    never render it. Refining to this tile is equivalent to refining
 *    to its children.
 * 2. Treat it as an indication that nothing need be rendered in this
 *    area at this level-of-detail. In other words, "render" it as a
 *    hole. To have this behavior, the tile should _not_ have content at
 *    all.
 *
 * We distinguish whether the tileset creator wanted (1) or (2) by
 * comparing this tile's geometricError to the geometricError of its
 * parent tile. If this tile's error is greater than or equal to its
 * parent, treat it as (1). If it's less, treat it as (2).
 *
 * For a tile with no parent there's no difference between the
 * behaviors.
 */
struct CESIUM3DTILESSELECTION_API TileEmptyContent {};

/**
 * @brief A content tag that indicates a tile content points to an
 * external tileset. When this tile is loaded, all the tiles in the
 * external tileset will become children of this external content tile
 */
struct CESIUM3DTILESSELECTION_API TileExternalContent {
  /**
   * @brief The metadata associated with this tileset.
   */
  TilesetMetadata metadata;
};

/**
 * @brief A content tag that indicates a tile has a glTF model content and
 * render resources for the model
 */
class CESIUM3DTILESSELECTION_API TileRenderContent {
public:
  /**
   * @brief Construct the content with a glTF model
   *
   * @param model A glTF model that will be owned by this content
   */
  TileRenderContent(CesiumGltf::Model&& model);

  /**
   * @brief Retrieve a glTF model that is owned by this content
   *
   * @return A glTF model that is owned by this content
   */
  const CesiumGltf::Model& getModel() const noexcept;

  /**
   * @brief Retrieve a glTF model that is owned by this content
   *
   * @return A glTF model that is owned by this content
   */
  CesiumGltf::Model& getModel() noexcept;

  /**
   * @brief Set the glTF model for this content
   *
   * @param model A glTF model that will be owned by this content
   */
  void setModel(const CesiumGltf::Model& model);

  /**
   * @brief Set the glTF model for this content
   *
   * @param model A glTF model that will be owned by this content
   */
  void setModel(CesiumGltf::Model&& model);

  /**
   * @brief Get the {@link CesiumRasterOverlays::RasterOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @return The {@link CesiumRasterOverlays::RasterOverlayDetails} that is owned by this content
   */
  const CesiumRasterOverlays::RasterOverlayDetails&
  getRasterOverlayDetails() const noexcept;

  /**
   * @brief Get the {@link CesiumRasterOverlays::RasterOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @return The {@link CesiumRasterOverlays::RasterOverlayDetails} that is owned by this content
   */
  CesiumRasterOverlays::RasterOverlayDetails&
  getRasterOverlayDetails() noexcept;

  /**
   * @brief Set the {@link CesiumRasterOverlays::RasterOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @param rasterOverlayDetails The {@link CesiumRasterOverlays::RasterOverlayDetails} that will be owned by this content
   */
  void setRasterOverlayDetails(
      const CesiumRasterOverlays::RasterOverlayDetails& rasterOverlayDetails);

  /**
   * @brief Set the {@link CesiumRasterOverlays::RasterOverlayDetails} which is the result of generating raster overlay UVs for the glTF model
   *
   * @param rasterOverlayDetails The {@link CesiumRasterOverlays::RasterOverlayDetails} that will be owned by this content
   */
  void setRasterOverlayDetails(
      CesiumRasterOverlays::RasterOverlayDetails&& rasterOverlayDetails);

  /**
   * @brief Get the list of \ref CesiumUtility::Credit "Credit" of the content
   *
   * @return The list of \ref CesiumUtility::Credit "Credit" of the content
   */
  const std::vector<CesiumUtility::Credit>& getCredits() const noexcept;

  /**
   * @brief Get the list of \ref CesiumUtility::Credit "Credit" of the content
   *
   * @return The list of \ref CesiumUtility::Credit "Credit" of the content
   */
  std::vector<CesiumUtility::Credit>& getCredits() noexcept;

  /**
   * @brief Set the list of \ref CesiumUtility::Credit "Credit" for the content
   *
   * @param credits The list of \ref CesiumUtility::Credit "Credit" to be owned
   * by the content
   */
  void setCredits(std::vector<CesiumUtility::Credit>&& credits);

  /**
   * @brief Set the list of \ref CesiumUtility::Credit "Credit" for the content
   *
   * @param credits The list of \ref CesiumUtility::Credit "Credit" to be owned
   * by the content
   */
  void setCredits(const std::vector<CesiumUtility::Credit>& credits);

  /**
   * @brief Get the render resources created for the glTF model of the content
   *
   * @return The render resources that is created for the glTF model
   */
  void* getRenderResources() const noexcept;

  /**
   * @brief Set the render resources created for the glTF model of the content
   *
   * @param pRenderResources The render resources that is created for the glTF
   * model
   */
  void setRenderResources(void* pRenderResources) noexcept;

  /**
   * @brief Get the fade percentage that this tile during an LOD transition.
   *
   * This will be used when {@link TilesetOptions::enableLodTransitionPeriod}
   * is true. Tile fades can be used to make LOD transitions appear less abrupt
   * and jarring. It is up to client implementations how to render the fade
   * percentage, but dithered fading is recommended.
   *
   * @return The fade percentage.
   */
  float getLodTransitionFadePercentage() const noexcept;

  /**
   * @brief Set the fade percentage of this tile during an LOD transition with.
   * Not to be used by clients.
   *
   * @param percentage The new fade percentage.
   */
  void setLodTransitionFadePercentage(float percentage) noexcept;

private:
  CesiumGltf::Model _model;
  void* _pRenderResources;
  CesiumRasterOverlays::RasterOverlayDetails _rasterOverlayDetails;
  std::vector<CesiumUtility::Credit> _credits;
  float _lodTransitionFadePercentage;
};

/**
 * @brief A tile content container that can store and query the content type
 * that is currently being owned by the tile
 */
class CESIUM3DTILESSELECTION_API TileContent {
  using TileContentKindImpl = std::variant<
      TileUnknownContent,
      TileEmptyContent,
      std::unique_ptr<TileExternalContent>,
      std::unique_ptr<TileRenderContent>>;

public:
  /**
   * @brief Construct an unknown content for a tile. This constructor
   * is useful when the tile content is known after its content is downloaded by
   * {@link TilesetContentLoader}
   */
  TileContent();

  /**
   * @brief Construct an empty content for a tile
   */
  TileContent(TileEmptyContent content);

  /**
   * @brief Construct an external content for a tile whose content
   * points to an external tileset
   */
  TileContent(std::unique_ptr<TileExternalContent>&& content);

  /**
   * @brief Set an unknown content tag for a tile. This constructor
   * is useful when the tile content is known after its content is downloaded by
   * {@link TilesetContentLoader}
   */
  void setContentKind(TileUnknownContent content);

  /**
   * @brief Construct an empty content tag for a tile
   */
  void setContentKind(TileEmptyContent content);

  /**
   * @brief Set an external content for a tile whose content
   * points to an external tileset
   */
  void setContentKind(std::unique_ptr<TileExternalContent>&& content);

  /**
   * @brief Set a glTF model content for a tile
   */
  void setContentKind(std::unique_ptr<TileRenderContent>&& content);

  /**
   * @brief Query if a tile has an unknown content
   */
  bool isUnknownContent() const noexcept;

  /**
   * @brief Query if a tile has an empty content
   */
  bool isEmptyContent() const noexcept;

  /**
   * @brief Query if a tile has an external content which points to
   * an external tileset
   */
  bool isExternalContent() const noexcept;

  /**
   * @brief Query if a tile has an glTF model content
   */
  bool isRenderContent() const noexcept;

  /**
   * @brief Get the {@link TileRenderContent} which stores the glTF model
   * and render resources of the tile
   */
  const TileRenderContent* getRenderContent() const noexcept;

  /**
   * @brief Get the {@link TileRenderContent} which stores the glTF model
   * and render resources of the tile
   */
  TileRenderContent* getRenderContent() noexcept;

  /**
   * @brief Get the {@link TileExternalContent} which stores the details of
   * the external tileset.
   */
  const TileExternalContent* getExternalContent() const noexcept;

  /**
   * @brief Get the {@link TileExternalContent} which stores the details of
   * the external tileset.
   */
  TileExternalContent* getExternalContent() noexcept;

private:
  TileContentKindImpl _contentKind;
};
} // namespace Cesium3DTilesSelection
