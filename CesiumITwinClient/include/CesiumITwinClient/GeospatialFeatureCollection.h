#pragma once

#include <CesiumGeospatial/BoundingRegion.h>

#include <string>
#include <utility>
#include <vector>

namespace CesiumITwinClient {

/**
 * @brief The extents of a feature collection.
 */
struct GeospatialFeatureCollectionExtents {
  /**
   * @brief A feature collection's spatial extents.
   *
   * From the [specification](https://docs.ogc.org/is/17-069r4/17-069r4.html):
   * > One or more bounding boxes that describe the spatial extent of the
   * > dataset. In the Core only a single bounding box is supported. Extensions
   * > may support additional areas. If multiple areas are provided, the union
   * > of the bounding boxes describes the spatial extent.
   */
  std::vector<CesiumGeometry::AxisAlignedBox> spatial;
  /**
   * @brief The coordinate reference system used for the spatial extents.
   *
   * Only a WGS84 CRS is currently supported.
   */
  std::string coordinateReferenceSystem;
  /**
   * @brief A feature collection's temporal extents.
   *
   * From the specification:
   * > One or more time intervals that describe the temporal extent of the
   * > dataset. The value `null` is supported and indicates an unbounded
   * > interval end. In the Core only a single time interval is supported.
   * > Extensions may support multiple intervals. If multiple intervals are
   * > provided, the union of the intervals describes the temporal extent.
   *
   * While the specification reads that this property includes "one or more time
   * intervals," that is only the case if the property is specified (it is
   * optional). If not specified, no time intervals will be listed.
   *
   * Null values will be represented as empty strings.
   */
  std::vector<std::pair<std::string, std::string>> temporal;
  /**
   * @brief The temporal reference system used for the temporal extents.
   *
   * The Gregorian calendar is currently the only supported TRS.
   */
  std::string temporalReferenceSystem;
};

/**
 * @brief The definition of a collection of geospatial features.
 */
struct GeospatialFeatureCollection {
  /**
   * @brief The ID of this feature collection.
   */
  std::string id;
  /**
   * @brief The title of this feature collection.
   */
  std::string title;
  /**
   * @brief The description of this feature collection.
   */
  std::string description;
  /**
   * @brief The spatial and temporal extents of this collection.
   */
  GeospatialFeatureCollectionExtents extents;
  /**
   * @brief The coordinate reference systems used by this feature collection.
   */
  std::vector<std::string> crs;
  /**
   * @brief The coordinate reference system used for storage.
   */
  std::string storageCrs;
  /**
   * @brief The epoch of the storage coordinate reference system, if applicable.
   */
  std::optional<std::string> storageCrsCoordinateEpoch;
};

} // namespace CesiumITwinClient