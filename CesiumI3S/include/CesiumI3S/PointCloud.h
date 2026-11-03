#pragma once
#include <CesiumI3S/AttributeStorage.h>
#include <CesiumI3S/DrawingInfo.h>
#include <CesiumI3S/Fields.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/GeometrySchema.h>
#include <CesiumI3S/Layer.h>
#include <CesiumI3S/Library.h>
#include <CesiumI3S/LodSelection.h>
#include <CesiumI3S/SpatialReference.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Vertex attribute declarations for PCSL geometry (I3S 2.0). */
struct CESIUMI3S_API PointCloudVertexAttribute {
  /** @brief XYZ position attribute. */
  Value position;
};

/** @brief Default geometry schema for a PCSL layer (I3S 2.0). */
struct CESIUMI3S_API PointCloudGeometrySchema {
  /** @brief Geometry type; always Point for PCSL. */
  GeometryTopology geometryType = GeometryTopology::Point;
  /** @brief Buffer layout; always PerAttributeArray for PCSL. */
  GeometrySchema::BufferLayout topology =
      GeometrySchema::BufferLayout::PerAttributeArray;
  /** @brief Geometry encoding; always Lepcc (LEPCC-XYZ) for PCSL. */
  Compression::Encoding encoding = Compression::Encoding::Lepcc;
  /** @brief Header attributes; always empty for PCSL. */
  std::vector<HeaderAttribute> header;
  std::vector<std::string> ordering = {
      /** @brief Vertex attribute ordering; only "position" is valid. */
      "position"};
  /** @brief Vertex attribute descriptors. */
  PointCloudVertexAttribute vertexAttributes;
};

/** @brief Bounding volume hierarchy index descriptor (index.pcsl.md, I3S
 * 2.0). */
struct CESIUMI3S_API PointCloudIndex {
  /** @brief Node version; must be 1. */
  uint32_t nodeVersion = 1;
  /** @brief Number of nodes per page. */
  uint32_t nodesPerPage = 64;
  /** @brief Bounding volume type; always OrientedBoundingBox for PCSL. */
  BoundingVolume boundingVolumeType = BoundingVolume::OrientedBoundingBox;
  /** @brief LoD metric; always DensityThreshold for PCSL. */
  LodSelection::Metric lodSelectionMetricType =
      LodSelection::Metric::DensityThreshold;
  /** @brief Relative URL to the node page index. */
  std::optional<std::string> href;
};

/** @brief Single node in the PCSL bounding volume hierarchy (node.pcsl.md,
 * I3S 2.0). */
struct CESIUMI3S_API PointCloudNode {
  /** @brief Index into the resource data arrays. */
  uint32_t resourceId = 0;
  /** @brief Index of the first child node. */
  uint32_t firstChild = 0;
  /** @brief Number of child nodes. */
  uint32_t childCount = 0;
  /** @brief Number of points in this node. */
  std::optional<uint32_t> vertexCount;
  /** @brief Oriented bounding box for this node. */
  OrientedBoundingBox obb;
  /** @brief LoD switching threshold. */
  std::optional<double> lodThreshold;
};

/** @brief Paged index document (nodePageDefinition.pcsl.md, I3S 2.0). */
struct CESIUMI3S_API PointCloudNodePage {
  /** @brief Array of nodes in this page. */
  std::vector<PointCloudNode> nodes;
};

/** @brief Store descriptor for a PCSL layer; profile is always "PointCloud"
 * (store.pcsl.md, I3S 2.0). */
struct CESIUMI3S_API PointCloudStore {
  /** @brief Unique identifier of the store. */
  std::optional<std::string> id;
  /** @brief I3S version string. */
  std::string version;
  /** @brief 2D geographic extent of the store. */
  Extent extent{};
  /** @brief Bounding volume hierarchy index descriptor. */
  PointCloudIndex index;
  /** @brief Default geometry buffer layout. */
  PointCloudGeometrySchema defaultGeometrySchema;
  /** @brief MIME type for geometry data (deprecated). */
  std::optional<std::string> geometryEncoding;
  /** @brief MIME type for attribute data (deprecated). */
  std::optional<std::string> attributeEncoding;
};

/** @brief Point Cloud Scene Layer definition; layerType is always "PointCloud"
 * (layer.pcsl.md, I3S 2.0). */
struct CESIUMI3S_API PointCloudLayer {
  /** @brief Unique layer ID within the service. */
  uint32_t id = 0;
  /** @brief Layer name. */
  std::string name;
  /** @brief Alias name for display. */
  std::optional<std::string> alias;
  /** @brief Layer description. */
  std::optional<std::string> description;
  /** @brief Copyright and usage information. */
  std::optional<std::string> copyrightText;
  /** @brief Operations supported by the layer. */
  std::vector<LayerCapabilities> capabilities;
  /** @brief Spatial reference of the layer. */
  SpatialReference spatialReference;
  /** @brief Height model metadata. */
  std::optional<HeightModelInfo> heightModelInfo;
  /** @brief Timestamp of the last service update. */
  std::optional<ServiceUpdateTimeStamp> serviceUpdateTimeStamp;
  /** @brief Store descriptor for the layer. */
  PointCloudStore store;
  /** @brief Attribute descriptors. */
  std::vector<AttributeStorageInfo> attributeStorageInfo;
  /** @brief Renderer definition for the layer. */
  std::optional<DrawingInfo> drawingInfo;
  /** @brief Elevation placement rules. */
  std::optional<ElevationInfo> elevationInfo;
  /** @brief Attribute field descriptors. */
  std::optional<std::vector<Field>> fields;
};

} // namespace CesiumI3S
