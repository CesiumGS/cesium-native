#pragma once

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/Library.h>
#include <CesiumVectorData/VectorStyle.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace CesiumVectorData {

/**
 * @brief Options for picking geometry at a position with
 * \ref GeoJsonSpatialIndex::pick.
 */
struct GeoJsonPickOptions {
  /**
   * @brief How far from the position, in meters, geometry may be and still be
   * picked.
   *
   * This is added to the drawn width of lines, polygon outlines and points.
   * Negative values are treated as zero.
   */
  double toleranceMeters = 0.0;

  /**
   * @brief The size of a screen pixel at the picked position, in meters.
   *
   * When set, lines, outlines and points whose size is given in pixels are
   * tested at the size they are drawn at. When not set, their pixel size is
   * ignored and only `toleranceMeters` (and any size given in meters) counts.
   */
  std::optional<double> metersPerPixel = std::nullopt;

  /**
   * @brief The maximum number of results to return.
   */
  size_t maxResults = 16;
};

/**
 * @brief A geometry object found by \ref GeoJsonSpatialIndex::pick.
 */
struct GeoJsonPickResult {
  /**
   * @brief The geometry object that was picked.
   *
   * This is always a `Point`, `MultiPoint`, `LineString`, `MultiLineString`,
   * `Polygon` or `MultiPolygon`.
   */
  const GeoJsonObject* pGeometry = nullptr;

  /**
   * @brief The `Feature` that contains \ref pGeometry, or `nullptr` if the
   * geometry is not part of a `Feature`.
   *
   * Use `pFeature->get<GeoJsonFeature>()` to access its ID and properties.
   */
  const GeoJsonObject* pFeature = nullptr;

  /**
   * @brief The distance in meters from the picked position to the geometry.
   *
   * This is measured to the center line of lines and outlines and to the
   * center of points, so it does not include the drawn width. It is zero
   * when the position is inside a polygon, and always zero for results of
   * picking a rectangle.
   */
  double distanceMeters = 0.0;
};

/**
 * @brief A spatial index over the geometry of a \ref GeoJsonDocument, used to
 * find the geometry and features at a position or within a rectangle.
 *
 * Hit testing matches how \ref VectorRasterizer draws the document: edges are
 * straight lines in longitude and latitude, polygon holes are excluded, and
 * the width of lines, polygon outlines and points is taken from each
 * geometry's style. A geometry's style is resolved from the geometry itself,
 * then from the `Feature` and collections that contain it, and finally from
 * the default style, in the same way as `GeoJsonDocumentRasterOverlay`.
 *
 * Only size matters for picking. A polygon's interior can be picked whether or
 * not it is filled, and color and transparency are ignored.
 *
 * Distances are measured in a plane tangent to the ellipsoid at the picked
 * position, using the ellipsoid's equatorial radius. This is accurate for the
 * small distances that picking deals with.
 *
 * Styles and geometry are read when the index is constructed. Changing a
 * style afterward does not affect the index; construct a new one instead.
 * The document's geometry must not be changed while the index exists.
 *
 * Once constructed, the index is not modified, so `pick` may be called
 * concurrently from multiple threads.
 */
class CESIUMVECTORDATA_API GeoJsonSpatialIndex {
public:
  /**
   * @brief Builds an index over all of the geometry in a document.
   *
   * @param pDocument The document to index. The index keeps it alive, and
   * picked results point into it.
   * @param defaultStyle The style for geometry that has no style of its own,
   * and is not contained in a `Feature` or collection that has one.
   * @param ellipsoid The ellipsoid the document is drawn on.
   */
  GeoJsonSpatialIndex(
      const std::shared_ptr<const GeoJsonDocument>& pDocument,
      const VectorStyle& defaultStyle = VectorStyle(),
      const CesiumGeospatial::Ellipsoid& ellipsoid =
          CesiumGeospatial::Ellipsoid::WGS84);

  /**
   * @brief Returns the document this index was built from.
   */
  const std::shared_ptr<const GeoJsonDocument>& getDocument() const noexcept {
    return this->_pDocument;
  }

  /**
   * @brief Returns the number of geometry objects in the index.
   */
  size_t size() const noexcept { return this->_entries.size(); }

  /**
   * @brief Finds the geometry at a position.
   *
   * Points are returned first, then lines, then polygons, so that geometry
   * drawn on top of a polygon's fill comes before the polygon. Within each
   * group, nearer geometry comes first, and geometry at the same distance is
   * returned in document order.
   *
   * @param position The position to pick at. Its height is ignored.
   * @param options The options to use.
   * @returns The picked geometry, at most `options.maxResults` of them.
   */
  std::vector<GeoJsonPickResult> pick(
      const CesiumGeospatial::Cartographic& position,
      const GeoJsonPickOptions& options = GeoJsonPickOptions()) const;

  /**
   * @brief Finds the geometry that lies at least partly within a rectangle.
   *
   * The geometry itself is tested, without its drawn width. A polygon is
   * returned if its interior or edges overlap the rectangle. The rectangle may
   * cross the antimeridian.
   *
   * @param rectangle The rectangle to search.
   * @param maxResults The maximum number of results to return.
   * @returns The geometry found, in document order.
   */
  std::vector<GeoJsonPickResult> pick(
      const CesiumGeospatial::GlobeRectangle& rectangle,
      size_t maxResults = std::numeric_limits<size_t>::max()) const;

private:
  enum class Kind : uint8_t { Point = 0, Line = 1, Polygon = 2 };

  struct Box {
    double west;
    double south;
    double east;
    double north;
  };

  struct Entry {
    const GeoJsonObject* pGeometry;
    const GeoJsonObject* pFeature;
    Kind kind;
    Box box;
    double halfWidthMeters;
    double halfWidthPixels;
  };

  struct Node {
    Box box;
    uint32_t first;
    uint32_t count;
    bool isLeaf;
  };

  void addObject(
      const GeoJsonObject& object,
      const VectorStyle& parentStyle,
      const GeoJsonObject* pFeature);
  void addGeometry(
      const GeoJsonObject& object,
      const VectorStyle& style,
      const GeoJsonObject* pFeature);
  void buildTree();

  template <typename Callback>
  void forEachCandidate(const Box& box, Callback&& callback) const;

  std::shared_ptr<const GeoJsonDocument> _pDocument;
  double _radius;
  double _maxHalfWidthMeters = 0.0;
  double _maxHalfWidthPixels = 0.0;
  std::vector<Entry> _entries;
  std::vector<Node> _nodes;
  std::vector<uint32_t> _leafEntries;
};

} // namespace CesiumVectorData
