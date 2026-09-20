#pragma once
#include <CesiumI3S/GeometrySchema.h>
#include <CesiumI3S/Library.h>
#include <CesiumI3S/Material.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3S/TextureDefinition.h>

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Store descriptor for the entire scene layer (store.cmn.md). */
struct CESIUMI3S_API Store {
  /** @brief Scene layer profile. */
  enum class Profile { Point, MeshPyramid, PointCloud };
  /** @brief Resource types present in the store. */
  enum class ResourcePattern {
    NodeIndexDocument,
    SharedResource,
    FeatureData,
    Geometry,
    Texture,
    Attributes
  };
  /** @brief Reference frame for vertex normals. */
  enum class NormalReferenceFrame {
    EastNorthUp,
    EarthCentered,
    VertexReferenceFrame
  };
  /** @brief LoD generation scheme (deprecated, I3S 1.7). */
  enum class LodType { MeshPyramid, AutoThinning, Clustering, Generalizing };
  /** @brief LoD model (deprecated, I3S 1.7). */
  enum class LodModel { NodeSwitching, None };

  /** @brief Unique identifier of the store. */
  std::optional<std::string> id;
  /** @brief Scene layer profile type. */
  Profile profile = Profile::MeshPyramid;
  /** @brief Resource types available in the store. */
  std::optional<std::vector<ResourcePattern>> resourcePattern;
  /** @brief Relative URL to the root node document. */
  std::optional<std::string> rootNode;
  /** @brief I3S version string. */
  std::string version;
  /** @brief 2D geographic extent of the store. */
  std::optional<Extent> extent;
  /** @brief CRS used for node bounding volumes and index. */
  std::optional<std::string> indexCrs;
  /** @brief CRS used for vertex positions. */
  std::optional<std::string> vertexCrs;
  /** @brief Reference frame for vertex normals. */
  std::optional<NormalReferenceFrame> normalReferenceFrame;
  /** @brief MIME type for node index documents (deprecated, I3S 1.6). */
  std::optional<std::string> nidEncoding;
  /** @brief MIME type for feature data (deprecated, I3S 1.6). */
  std::optional<std::string> featureEncoding;
  /** @brief MIME type for geometry data (deprecated, I3S 1.6). */
  std::optional<std::string> geometryEncoding;
  /** @brief MIME type for attribute data (deprecated, I3S 1.6). */
  std::optional<std::string> attributeEncoding;
  /** @brief MIME types for texture data (deprecated, I3S 1.6). */
  std::optional<std::vector<std::string>> textureEncoding;
  /** @brief LoD generation scheme (deprecated, I3S 1.7). */
  std::optional<LodType> lodType;
  /** @brief LoD model (deprecated, I3S 1.7). */
  std::optional<LodModel> lodModel;
  /** @brief Indexing scheme (deprecated, I3S 1.7). */
  std::optional<std::string> indexingScheme;
  /** @brief Default geometry buffer layout (required for meshpyramid profile).
   */
  GeometrySchema defaultGeometrySchema;
  /** @brief Default texture definitions (deprecated, I3S 1.7). */
  std::optional<std::vector<TextureDefinitionInfo>> defaultTextureDefinition;
  /** @brief Default material definitions (deprecated, I3S 1.7). */
  std::optional<MaterialDefinitionMap> defaultMaterialDefinition;
};

} // namespace CesiumI3S
