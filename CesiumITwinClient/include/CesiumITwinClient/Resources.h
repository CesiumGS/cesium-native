#pragma once

#include <CesiumGeospatial/GlobeRectangle.h>

#include <cstdint>
#include <string>

namespace CesiumITwinClient {

/**
 * @brief Information on a user.
 */
struct UserProfile {
  /**
   * @brief A GUID representing this user.
   */
  std::string id;
  /**
   * @brief A display name for this user, often the same as `email`.
   */
  std::string displayName;
  /**
   * @brief The user's first name.
   */
  std::string givenName;
  /**
   * @brief The user's last name.
   */
  std::string surname;
  /**
   * @brief The user's email.
   */
  std::string email;
};

/**
 * @brief The type of content obtained from the iTwin Cesium Curated Content
 * API.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contenttype
 * for more information.
 */
enum class ITwinCesiumCuratedContentType : uint8_t {
  /** @brief The content type returned is not a known type. */
  Unknown = 0,
  /** @brief The content is a 3D Tiles tileset. */
  Cesium3DTiles = 1,
  /** @brief The content is a glTF model. */
  Gltf = 2,
  /**
   * @brief The content is imagery that can be loaded with \ref
   * CesiumRasterOverlays::ITwinCesiumCuratedContentRasterOverlay.
   */
  Imagery = 3,
  /** @brief The content is quantized mesh terrain. */
  Terrain = 4,
  /** @brief The content is in the Keyhole Markup Language (KML) format. */
  Kml = 5,
  /**
   * @brief The content is in the Cesium Language (CZML) format.
   *
   * See https://github.com/AnalyticalGraphicsInc/czml-writer/wiki/CZML-Guide
   * for more information.
   */
  Czml = 6,
  /** @brief The content is in the GeoJSON format. */
  GeoJson = 7,
};

/**
 * @brief Obtains a \ref ITwinCesiumCuratedContentType value from the provided
 * string.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contenttype
 * for the list of possible values.
 */
ITwinCesiumCuratedContentType
cesiumCuratedContentTypeFromString(const std::string& str);

/**
 * @brief Describes the state of the content during the upload and tiling
 * processes.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contentstatus
 * for more information.
 */
enum class ITwinCesiumCuratedContentStatus : uint8_t {
  Unknown = 0,
  AwaitingFiles = 1,
  NotStarted = 2,
  InProgress = 3,
  Complete = 4,
  Error = 5,
  DataError = 6
};

/**
 * @brief Obtains a \ref ITwinCesiumCuratedContentStatus value from the provided
 * string.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contentstatus
 * for the list of possible values.
 */
ITwinCesiumCuratedContentStatus
cesiumCuratedContentStatusFromString(const std::string& str);

/**
 * @brief A single item obtained from the iTwin Cesium Curated Content API.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#content
 * for more information.
 */
struct ITwinCesiumCuratedContentItem {
  /** @brief The unique identifier for the content. */
  uint64_t id;
  /** @brief The type of the content. */
  ITwinCesiumCuratedContentType type;
  /** @brief The name of the content. */
  std::string name;
  /** @brief A Markdown string describing the content. */
  std::string description;
  /**
   * @brief A Markdown compatible string containing any required attribution for
   * the content.
   *
   * Clients will be required to display the attribution to end users.
   */
  std::string attribution;
  /** @brief The status of the content. */
  ITwinCesiumCuratedContentStatus status;
};

/**
 * @brief The status of the iTwin.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwinstatus
 * for more information.
 */
enum class ITwinStatus : uint8_t {
  Unknown = 0,
  Active = 1,
  Inactive = 2,
  Trial = 3
};

/**
 * @brief Obtains an \ref ITwinStatus value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwinstatus
 * for a list of possible values.
 */
ITwinStatus iTwinStatusFromString(const std::string& str);

/**
 * @brief Information on a single iTwin.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwin-summary
 * for more information.
 */
struct ITwin {
  /** @brief The iTwin Id. */
  std::string id;
  /**
   * @brief The `Class` of your iTwin.
   *
   * See
   * https://developer.bentley.com/apis/itwins/overview/#itwin-classes-and-subclasses
   * for more information.
   */
  std::string iTwinClass;
  /**
   * @brief The `subClass` of your iTwin.
   *
   * See
   * https://developer.bentley.com/apis/itwins/overview/#itwin-classes-and-subclasses
   * for more information.
   */
  std::string subClass;
  /** @brief An open ended property to better define your iTwin's Type. */
  std::string type;
  /**
   * @brief A unique number or code for the iTwin.
   *
   * This is the value that uniquely identifies the iTwin within your
   * organization.
   */
  std::string number;
  /** @brief A display name for the iTwin. */
  std::string displayName;
  /** @brief The status of the iTwin. */
  ITwinStatus status;
};

/**
 * @brief The possible states for an iModel.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel-state
 * for more information.
 */
enum class IModelState : uint8_t {
  Unknown = 0,
  Initialized = 1,
  NotInitialized = 2
};

/**
 * @brief Obtains an \ref IModelState value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel-state
 * for a list of possible values.
 */
IModelState iModelStateFromString(const std::string& str);

/**
 * @brief An iModel.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel
 * for more information.
 */
struct IModel {
  /** @brief Id of the iModel. */
  std::string id;
  /** @brief Display name of the iModel. */
  std::string displayName;
  /** @brief Name of the iModel. */
  std::string name;
  /** @brief Description of the iModel. */
  std::string description;
  /** @brief Indicates the state of the iModel. */
  IModelState state;
  /**
   * @brief The maximum rectangular area on the Earth which encloses the iModel.
   */
  CesiumGeospatial::GlobeRectangle extent;
};

/**
 * @brief The status of an iModel Mesh Export.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#exportstatus
 * for more information.
 */
enum class IModelMeshExportStatus : uint8_t {
  Unknown = 0,
  NotStarted = 1,
  InProgress = 2,
  Complete = 3,
  Invalid = 4
};

/**
 * @brief Obtains an \ref IModelMeshExportStatus value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#exportstatus
 * for a list of possible values.
 */
IModelMeshExportStatus iModelMeshExportStatusFromString(const std::string& str);

/**
 * @brief The type of mesh exported.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#startexport-exporttype
 * for more information.
 */
enum class IModelMeshExportType : uint8_t {
  Unknown = 0,
  /**
   * @brief iTwin "3D Fast Transmission" (3DFT) format.
   */
  ITwin3DFT = 1,
  IModel = 2,
  Cesium = 3,
  Cesium3DTiles = 4,
};

/**
 * @brief Obtains an \ref IModelMeshExportType value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#startexport-exporttype
 * for a list of possible values.
 */
IModelMeshExportType iModelMeshExportTypeFromString(const std::string& str);

/**
 * @brief An iModel mesh export.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#export
 * for more information.
 */
struct IModelMeshExport {
  /** @brief ID of the export request. */
  std::string id;
  /** @brief Name of the exported iModel. */
  std::string displayName;
  /** @brief The status of the export job. */
  IModelMeshExportStatus status;
  /** @brief The type of the export. */
  IModelMeshExportType exportType;
};

/**
 * @brief Indicates the nature of the reality data.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/rm-rd-details/#classification
 * for more information.
 */
enum class ITwinRealityDataClassification : uint8_t {
  Unknown = 0,
  Terrain = 1,
  Imagery = 2,
  Pinned = 3,
  Model = 4,
  Undefined = 5
};

/**
 * @brief Obtains an \ref ITwinRealityDataClassification value from the provided
 * string.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/rm-rd-details/#classification
 * for a list of possible values.
 */
ITwinRealityDataClassification
iTwinRealityDataClassificationFromString(const std::string& str);

/**
 * @brief Information on reality data.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/operations/get-all-reality-data/#reality-data-metadata
 * for more information.
 */
struct ITwinRealityData {
  /**
   * @brief Identifier of the reality data.
   *
   * This identifier is assigned by the service at the creation of the reality
   * data. It is also unique.
   */
  std::string id;
  /**
   * @brief The name of the reality data.
   *
   * This property may not contain any control sequence such as a URL or code.
   */
  std::string displayName;
  /**
   * @brief A textual description of the reality data.
   *
   * This property may not contain any control sequence such as a URL or code.
   */
  std::string description;
  /**
   * @brief Specific value constrained field that indicates the nature of the
   * reality data.
   */
  ITwinRealityDataClassification classification;
  /**
   * @brief A key indicating the format of the data.
   *
   * The type property should be a specific indication of the format of the
   * reality data. Given a type, the consuming software should be able to
   * determine if it has the capacity to open the reality data. Although the
   * type field is a free string some specific values are reserved and other
   * values should be selected judiciously. Look at the documentation for <a
   * href="https://developer.bentley.com/apis/reality-management/rm-rd-details/#types">an
   * exhaustive list of reserved reality data types</a>.
   */
  std::string type;
  /**
   * @brief Contains the rectangular area on the Earth which encloses the
   * reality data.
   */
  CesiumGeospatial::GlobeRectangle extent;
  /**
   * @brief A boolean value that is true if the data is being created. It is
   * false if the data has been completely uploaded.
   */
  bool authoring;
};

/**
 * @brief The API from which an `ITwinResource` originated.
 */
enum class ResourceSource : uint8_t {
  /** @brief This resource represents Cesium Curated Content. */
  CesiumCuratedContent = 1,
  /** @brief This resource represents an iModel Mesh Export. */
  MeshExport = 2,
  /** @brief This resource represents Reality Data. */
  RealityData = 3
};

/**
 * @brief The type of resource that an `ITwinResource` represents.
 */
enum class ResourceType : uint8_t {
  /** @brief This resource is a 3D Tiles tileset. */
  Tileset = 1,
  /** @brief This resource is raster overlay imagery. */
  Imagery = 2,
  /** @brief This resource is quantized mesh terrain. */
  Terrain = 3
};

/**
 * @brief A single resource obtained from the iTwin API that can be loaded by
 * Cesium Native.
 */
struct ITwinResource {
  /**
   * @brief The ID of this resource.
   *
   * - If `source` is `CesiumCuratedContent`, this is the integer ID of the
   * asset.
   * - If `source` is `MeshExport`, this is the GUID of the mesh export.
   * - If `source` is `RealityData`, this is the GUID of the reality data.
   */
  std::string id;
  /**
   * @brief The GUID of this resource's parent, if any.
   *
   * - If `source` is `CesiumCuratedContent`, this is `std::nullopt`.
   * - If `source` is `MeshExport`, this is the GUID of the iModel.
   * - If `source` is `RealityData`, this is the GUID of the iTwin.
   */
  std::optional<std::string> parentId;
  /**
   * @brief The API from which this resource originated.
   */
  ResourceSource source;
  /** @brief The display name of this resource. */
  std::string displayName;
  /** @brief The type of this resource. */
  ResourceType resourceType;
};

} // namespace CesiumITwinClient
