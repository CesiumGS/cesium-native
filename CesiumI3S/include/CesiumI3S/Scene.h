#pragma once
#include <CesiumI3S/AttributeStorage.h>
#include <CesiumI3S/Building.h>
#include <CesiumI3S/DrawingInfo.h>
#include <CesiumI3S/Fields.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/Layer.h>
#include <CesiumI3S/Library.h>
#include <CesiumI3S/Material.h>
#include <CesiumI3S/Node.h>
#include <CesiumI3S/PointCloud.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3S/Stats.h>
#include <CesiumI3S/Store.h>
#include <CesiumI3S/TextureDefinition.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace CesiumI3S {

/** @brief I3S scene layer type. */
enum class SceneLayerType {
  ThreeDObject,
  IntegratedMesh,
  Point,
  PointCloud,
  Building
};

/** @brief Temporal metadata for a time-aware layer (timeInfo.cmn.md). */
struct CESIUMI3S_API TimeInfo {
  /** @brief Name of the field holding the end time value. */
  std::optional<std::string> endTimeField;
  /** @brief Name of the field holding the start time value. */
  std::optional<std::string> startTimeField;
};

/** @brief Range filter metadata for a layer (rangeInfo.cmn.md). */
struct CESIUMI3S_API RangeInfo {
  /** @brief Name of the attribute field used for range filtering. */
  std::string field;
  /** @brief Display name of the range slider. */
  std::string name;
};

/** @brief 3D Scene Layer definition (3DSceneLayer.cmn.md, 3DSceneLayer.psl.md).
 */
struct CESIUMI3S_API Layer {
  /** @brief Unique layer ID within the service. */
  uint32_t id = 0;
  /** @brief Relative URL to the layer resource. */
  std::optional<std::string> href;
  /** @brief Scene layer type. */
  SceneLayerType layerType = SceneLayerType::ThreeDObject;
  /** @brief Spatial reference of the layer. */
  std::optional<SpatialReference> spatialReference;
  /** @brief Height model metadata. */
  std::optional<HeightModelInfo> heightModelInfo;
  /** @brief Version of the layer (store update session ID). */
  std::string version;
  /** @brief Layer name. */
  std::optional<std::string> name;
  /** @brief Alias name for display. */
  std::optional<std::string> alias;
  /** @brief Layer description. */
  std::optional<std::string> description;
  /** @brief Copyright and usage information. */
  std::optional<std::string> copyrightText;
  /** @brief Operations supported by the layer. */
  std::vector<LayerCapabilities> capabilities;
  /** @brief Indicates whether drawingInfo colour is captured in binary. */
  std::optional<CachedDrawingInfo> cachedDrawingInfo;
  /** @brief Renderer definition for the layer. */
  std::optional<DrawingInfo> drawingInfo;
  /** @brief Elevation placement rules. */
  std::optional<ElevationInfo> elevationInfo;
  /** @brief Timestamp of the last service update. */
  std::optional<ServiceUpdateTimeStamp> serviceUpdateTimeStamp;
  /** @brief Store descriptor for the layer. */
  Store store;
  /** @brief Attribute field descriptors. */
  std::optional<std::vector<Field>> fields;
  /** @brief Binary attribute storage descriptors. */
  std::optional<std::vector<AttributeStorageInfo>> attributeStorageInfo;
  /** @brief References to per-field statistics resources. */
  std::optional<std::vector<StatisticsInfo>> statisticsInfo;
  /** @brief Node page definition (I3S 1.7+). */
  std::optional<NodePageDefinition> nodePages;
  /** @brief Geometry buffer layout definitions. */
  std::vector<GeometryDefinition> geometryDefinitions;
  /** @brief PBR material definitions. */
  std::vector<MaterialDefinition> materialDefinitions;
  /** @brief Texture set definitions. */
  std::vector<TextureSetDefinition> textureSetDefinitions;
  /** @brief 3D geographic extent of the layer. */
  std::optional<FullExtent> fullExtent;
  /** @brief Scale factor applied to z-values. */
  std::optional<double> zFactor;
  /** @brief Popup configuration; raw JSON per ArcGIS Web Scene Specification.
   */
  std::optional<std::string> popupInfo;
  /** @brief When true, popups are disabled. Default=false. */
  bool disablePopup = false;
  /** @brief Temporal metadata for a time-aware layer. */
  std::optional<TimeInfo> timeInfo;
  /** @brief Range filter metadata. */
  std::optional<RangeInfo> rangeInfo;
};

/** @brief A discriminated union of all I3S scene layer types. */
using SceneLayer = std::variant<Layer, BuildingLayer, PointCloudLayer>;

/** @brief Root service document (SceneServiceInfo). */
struct CESIUMI3S_API SceneResource {
  /** @brief Service identifier. */
  std::string id;
  /** @brief Service version string. */
  std::string version;
  /** @brief All scene layers (CMN, PSL, PCSL, and BLD). */
  std::vector<SceneLayer> layers;
  /** @brief Base URL of the service. */
  std::optional<std::string> baseUrl;
  /** @brief Spatial reference of the service. */
  std::optional<SpatialReference> spatialReference;
};

} // namespace CesiumI3S
