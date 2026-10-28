#include "EnumJsonHandler.h"
#include "IEqual.h"

#include <CesiumI3S/AttributeStorage.h>
#include <CesiumI3S/Building.h>
#include <CesiumI3S/DrawingInfo.h>
#include <CesiumI3S/Fields.h>
#include <CesiumI3S/Geometry.h>
#include <CesiumI3S/GeometrySchema.h>
#include <CesiumI3S/Layer.h>
#include <CesiumI3S/LodSelection.h>
#include <CesiumI3S/Material.h>
#include <CesiumI3S/Node.h>
#include <CesiumI3S/PointCloud.h>
#include <CesiumI3S/Scene.h>
#include <CesiumI3S/SpatialReference.h>
#include <CesiumI3S/Store.h>
#include <CesiumI3S/TextureDefinition.h>

#include <algorithm>
#include <cctype>

namespace CesiumI3SReader {

template <>
std::optional<CesiumI3S::Renderer::Type>
enumFromString<CesiumI3S::Renderer::Type>(const std::string_view& str) {
  using E = CesiumI3S::Renderer::Type;
  if (iequal(str, "simple"))
    return E::Simple;
  if (iequal(str, "uniquevalue"))
    return E::UniqueValue;
  if (iequal(str, "classbreaks"))
    return E::ClassBreaks;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::SceneLayerType>
enumFromString<CesiumI3S::SceneLayerType>(const std::string_view& str) {
  using E = CesiumI3S::SceneLayerType;
  if (iequal(str, "3dobject"))
    return E::ThreeDObject;
  if (iequal(str, "integratedmesh"))
    return E::IntegratedMesh;
  if (iequal(str, "point"))
    return E::Point;
  if (iequal(str, "pointcloud"))
    return E::PointCloud;
  if (iequal(str, "building"))
    return E::Building;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::LayerCapabilities>
enumFromString<CesiumI3S::LayerCapabilities>(const std::string_view& str) {
  using E = CesiumI3S::LayerCapabilities;
  if (iequal(str, "view"))
    return E::View;
  if (iequal(str, "query"))
    return E::Query;
  if (iequal(str, "edit"))
    return E::Edit;
  if (iequal(str, "extract"))
    return E::Extract;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::HeightModel>
enumFromString<CesiumI3S::HeightModel>(const std::string_view& str) {
  using E = CesiumI3S::HeightModel;
  if (iequal(str, "gravity-related-height") ||
      iequal(str, "gravity_related_height"))
    return E::GravityRelatedHeight;
  if (iequal(str, "ellipsoidal"))
    return E::Ellipsoidal;
  if (iequal(str, "orthometric"))
    return E::Orthometric;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::HeightUnit>
enumFromString<CesiumI3S::HeightUnit>(const std::string_view& str) {
  using E = CesiumI3S::HeightUnit;
  if (iequal(str, "meter"))
    return E::Meter;
  if (iequal(str, "us-foot"))
    return E::UsFoot;
  if (iequal(str, "foot"))
    return E::Foot;
  if (iequal(str, "clarke-foot"))
    return E::ClarkeFoot;
  if (iequal(str, "clarke-yard"))
    return E::ClarkeYard;
  if (iequal(str, "clarke-link"))
    return E::ClarkeLink;
  if (iequal(str, "sears-yard"))
    return E::SearsYard;
  if (iequal(str, "sears-foot"))
    return E::SearsFoot;
  if (iequal(str, "sears-chain"))
    return E::SearsChain;
  if (iequal(str, "benoit-1895-b-chain"))
    return E::Benoit1895BChain;
  if (iequal(str, "indian-yard"))
    return E::IndianYard;
  if (iequal(str, "indian-1937-yard"))
    return E::Indian1937Yard;
  if (iequal(str, "gold-coast-foot"))
    return E::GoldCoastFoot;
  if (iequal(str, "sears-1922-truncated-chain"))
    return E::Sears1922TruncatedChain;
  if (iequal(str, "us-inch"))
    return E::UsInch;
  if (iequal(str, "us-mile"))
    return E::UsMile;
  if (iequal(str, "us-yard"))
    return E::UsYard;
  if (iequal(str, "millimeter"))
    return E::Millimeter;
  if (iequal(str, "decimeter"))
    return E::Decimeter;
  if (iequal(str, "centimeter"))
    return E::Centimeter;
  if (iequal(str, "kilometer"))
    return E::Kilometer;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::ElevationMode>
enumFromString<CesiumI3S::ElevationMode>(const std::string_view& str) {
  using E = CesiumI3S::ElevationMode;
  if (iequal(str, "relativetoground"))
    return E::RelativeToGround;
  if (iequal(str, "absoluteheight"))
    return E::AbsoluteHeight;
  if (iequal(str, "ontheground"))
    return E::OnTheGround;
  if (iequal(str, "relativetoscene"))
    return E::RelativeToScene;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Store::Profile>
enumFromString<CesiumI3S::Store::Profile>(const std::string_view& str) {
  using E = CesiumI3S::Store::Profile;
  if (iequal(str, "points"))
    return E::Point;
  if (iequal(str, "meshpyramid") || iequal(str, "meshpyramids"))
    return E::MeshPyramid;
  if (iequal(str, "pointcloud"))
    return E::PointCloud;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Store::ResourcePattern>
enumFromString<CesiumI3S::Store::ResourcePattern>(const std::string_view& str) {
  using E = CesiumI3S::Store::ResourcePattern;
  if (iequal(str, "nodeindexdocument") || iequal(str, "3dnodeindexdocument"))
    return E::NodeIndexDocument;
  if (iequal(str, "sharedresource"))
    return E::SharedResource;
  if (iequal(str, "featuredata"))
    return E::FeatureData;
  if (iequal(str, "geometry"))
    return E::Geometry;
  if (iequal(str, "texture"))
    return E::Texture;
  if (iequal(str, "attributes"))
    return E::Attributes;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Store::NormalReferenceFrame>
enumFromString<CesiumI3S::Store::NormalReferenceFrame>(
    const std::string_view& str) {
  using E = CesiumI3S::Store::NormalReferenceFrame;
  if (iequal(str, "east-north-up"))
    return E::EastNorthUp;
  if (iequal(str, "earth-centered"))
    return E::EarthCentered;
  if (iequal(str, "vertex-reference-frame"))
    return E::VertexReferenceFrame;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Store::LodType>
enumFromString<CesiumI3S::Store::LodType>(const std::string_view& str) {
  using E = CesiumI3S::Store::LodType;
  if (iequal(str, "meshpyramid"))
    return E::MeshPyramid;
  if (iequal(str, "autothinning"))
    return E::AutoThinning;
  if (iequal(str, "clustering"))
    return E::Clustering;
  if (iequal(str, "generalizing"))
    return E::Generalizing;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Store::LodModel>
enumFromString<CesiumI3S::Store::LodModel>(const std::string_view& str) {
  using E = CesiumI3S::Store::LodModel;
  if (iequal(str, "node-switching"))
    return E::NodeSwitching;
  if (iequal(str, "none"))
    return E::None;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::LodSelection::Metric>
enumFromString<CesiumI3S::LodSelection::Metric>(const std::string_view& str) {
  using E = CesiumI3S::LodSelection::Metric;
  if (iequal(str, "maxscreenthreshold"))
    return E::MaxScreenThreshold;
  if (iequal(str, "maxscreenthresholdsq"))
    return E::MaxScreenThresholdSQ;
  if (iequal(str, "screenspacerelative"))
    return E::ScreenSpaceRelative;
  if (iequal(str, "distancerangefromdefaultcamera"))
    return E::DistanceRangeFromDefaultCamera;
  if (iequal(str, "effectivedensity"))
    return E::EffectiveDensity;
  if (iequal(str, "density-threshold"))
    return E::DensityThreshold;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::AttributeStorageInfo::Ordering>
enumFromString<CesiumI3S::AttributeStorageInfo::Ordering>(
    const std::string_view& str) {
  using E = CesiumI3S::AttributeStorageInfo::Ordering;
  if (iequal(str, "attributebytecounts"))
    return E::AttributeByteCounts;
  if (iequal(str, "attributevalues"))
    return E::AttributeValues;
  if (iequal(str, "objectids"))
    return E::ObjectIds;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::DataType>
enumFromString<CesiumI3S::DataType>(const std::string_view& str) {
  using E = CesiumI3S::DataType;
  if (iequal(str, "int8"))
    return E::Int8;
  if (iequal(str, "int16"))
    return E::Int16;
  if (iequal(str, "int32"))
    return E::Int32;
  if (iequal(str, "int64"))
    return E::Int64;
  if (iequal(str, "uint8"))
    return E::UInt8;
  if (iequal(str, "uint16"))
    return E::UInt16;
  if (iequal(str, "uint32"))
    return E::UInt32;
  if (iequal(str, "uint64"))
    return E::UInt64;
  if (iequal(str, "float32"))
    return E::Float32;
  if (iequal(str, "float64"))
    return E::Float64;
  if (iequal(str, "string"))
    return E::String;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::HeaderValue::Property>
enumFromString<CesiumI3S::HeaderValue::Property>(const std::string_view& str) {
  using E = CesiumI3S::HeaderValue::Property;
  if (iequal(str, "count"))
    return E::Count;
  if (iequal(str, "attributevaluesbytecount"))
    return E::AttributeValuesByteCount;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Value::TimeEncoding>
enumFromString<CesiumI3S::Value::TimeEncoding>(const std::string_view& str) {
  using E = CesiumI3S::Value::TimeEncoding;
  if (iequal(str, "ecma-iso 8601"))
    return E::ECMAIS8601;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Field::Type>
enumFromString<CesiumI3S::Field::Type>(const std::string_view& str) {
  using E = CesiumI3S::Field::Type;
  if (iequal(str, "esrifieldtypedate"))
    return E::Date;
  if (iequal(str, "esrifieldtypesingle"))
    return E::Single;
  if (iequal(str, "esrifieldtypedouble"))
    return E::Double;
  if (iequal(str, "esrifieldtypeguid"))
    return E::GUID;
  if (iequal(str, "esrifieldtypeglobalid"))
    return E::GlobalID;
  if (iequal(str, "esrifieldtypeinteger"))
    return E::Integer;
  if (iequal(str, "esrifieldtypeoid"))
    return E::OID;
  if (iequal(str, "esrifieldtypesmallinteger"))
    return E::SmallInteger;
  if (iequal(str, "esrifieldtypestring"))
    return E::String;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Domain::Type>
enumFromString<CesiumI3S::Domain::Type>(const std::string_view& str) {
  using E = CesiumI3S::Domain::Type;
  if (iequal(str, "codedvalue"))
    return E::CodedValue;
  if (iequal(str, "range"))
    return E::Range;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Domain::FieldType>
enumFromString<CesiumI3S::Domain::FieldType>(const std::string_view& str) {
  using E = CesiumI3S::Domain::FieldType;
  if (iequal(str, "esrifieldtypedate"))
    return E::Date;
  if (iequal(str, "esrifieldtypesingle"))
    return E::Single;
  if (iequal(str, "esrifieldtypedouble"))
    return E::Double;
  if (iequal(str, "esrifieldtypeinteger"))
    return E::Integer;
  if (iequal(str, "esrifieldtypesmallinteger"))
    return E::SmallInteger;
  if (iequal(str, "esrifieldtypestring"))
    return E::String;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Domain::MergePolicy>
enumFromString<CesiumI3S::Domain::MergePolicy>(const std::string_view& str) {
  using E = CesiumI3S::Domain::MergePolicy;
  if (iequal(str, "mptdefaultvalue"))
    return E::MPTDefaultValue;
  if (iequal(str, "mptsumvalues"))
    return E::MPTSumValues;
  if (iequal(str, "mptareaweighted"))
    return E::MPTAreaWeighted;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Domain::SplitPolicy>
enumFromString<CesiumI3S::Domain::SplitPolicy>(const std::string_view& str) {
  using E = CesiumI3S::Domain::SplitPolicy;
  if (iequal(str, "sptgeometryratio"))
    return E::SPTGeometryRatio;
  if (iequal(str, "sptduplicate"))
    return E::SPTDuplicate;
  if (iequal(str, "sptdefaultvalue"))
    return E::SPTDefaultValue;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::MaterialDefinition::AlphaMode>
enumFromString<CesiumI3S::MaterialDefinition::AlphaMode>(
    const std::string_view& str) {
  using E = CesiumI3S::MaterialDefinition::AlphaMode;
  if (iequal(str, "opaque"))
    return E::Opaque;
  if (iequal(str, "mask"))
    return E::Mask;
  if (iequal(str, "blend"))
    return E::Blend;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::MaterialDefinition::CullFace>
enumFromString<CesiumI3S::MaterialDefinition::CullFace>(
    const std::string_view& str) {
  using E = CesiumI3S::MaterialDefinition::CullFace;
  if (iequal(str, "none"))
    return E::None;
  if (iequal(str, "front"))
    return E::Front;
  if (iequal(str, "back"))
    return E::Back;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::MaterialParams::RenderMode>
enumFromString<CesiumI3S::MaterialParams::RenderMode>(
    const std::string_view& str) {
  using E = CesiumI3S::MaterialParams::RenderMode;
  if (iequal(str, "textured"))
    return E::Textured;
  if (iequal(str, "solid"))
    return E::Solid;
  if (iequal(str, "untextured"))
    return E::Untextured;
  if (iequal(str, "wireframe"))
    return E::Wireframe;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::MaterialDefinitionInfo::Type>
enumFromString<CesiumI3S::MaterialDefinitionInfo::Type>(
    const std::string_view& str) {
  using E = CesiumI3S::MaterialDefinitionInfo::Type;
  if (iequal(str, "standard"))
    return E::Standard;
  if (iequal(str, "water"))
    return E::Water;
  if (iequal(str, "billboard"))
    return E::Billboard;
  if (iequal(str, "leafcard"))
    return E::Leafcard;
  if (iequal(str, "reference"))
    return E::Reference;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::TextureWrap>
enumFromString<CesiumI3S::TextureWrap>(const std::string_view& str) {
  using E = CesiumI3S::TextureWrap;
  if (iequal(str, "none"))
    return E::None;
  if (iequal(str, "repeat"))
    return E::Repeat;
  if (iequal(str, "mirror"))
    return E::Mirror;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::TextureChannels>
enumFromString<CesiumI3S::TextureChannels>(const std::string_view& str) {
  using E = CesiumI3S::TextureChannels;
  if (iequal(str, "rgb"))
    return E::Rgb;
  if (iequal(str, "rgba"))
    return E::Rgba;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::TextureFormat>
enumFromString<CesiumI3S::TextureFormat>(const std::string_view& str) {
  using E = CesiumI3S::TextureFormat;
  if (iequal(str, "jpg"))
    return E::Jpg;
  if (iequal(str, "png"))
    return E::Png;
  if (iequal(str, "dds"))
    return E::Dds;
  if (iequal(str, "ktx-etc2"))
    return E::KtxEtc2;
  if (iequal(str, "ktx2"))
    return E::Ktx2;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::GeometrySchema::BufferLayout>
enumFromString<CesiumI3S::GeometrySchema::BufferLayout>(
    const std::string_view& str) {
  using E = CesiumI3S::GeometrySchema::BufferLayout;
  if (iequal(str, "perattributearray"))
    return E::PerAttributeArray;
  if (iequal(str, "indexed"))
    return E::Indexed;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Compression::Encoding>
enumFromString<CesiumI3S::Compression::Encoding>(const std::string_view& str) {
  if (iequal(str, "draco"))
    return CesiumI3S::Compression::Encoding::Draco;
  if (iequal(str, "lepcc-xyz"))
    return CesiumI3S::Compression::Encoding::Lepcc;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::GeometryBuffer::Attribute::Binding>
enumFromString<CesiumI3S::GeometryBuffer::Attribute::Binding>(
    const std::string_view& str) {
  using E = CesiumI3S::GeometryBuffer::Attribute::Binding;
  if (iequal(str, "per-vertex"))
    return E::PerVertex;
  if (iequal(str, "per-feature"))
    return E::PerFeature;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::GeometryBuffer::FeatureIdAttribute::Type>
enumFromString<CesiumI3S::GeometryBuffer::FeatureIdAttribute::Type>(
    const std::string_view& str) {
  using E = CesiumI3S::GeometryBuffer::FeatureIdAttribute::Type;
  if (iequal(str, "uint16"))
    return E::UInt16;
  if (iequal(str, "uint32"))
    return E::UInt32;
  if (iequal(str, "uint64"))
    return E::UInt64;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Compression::Attribute>
enumFromString<CesiumI3S::Compression::Attribute>(const std::string_view& str) {
  using E = CesiumI3S::Compression::Attribute;
  if (iequal(str, "position"))
    return E::Position;
  if (iequal(str, "normal"))
    return E::Normal;
  if (iequal(str, "uv0"))
    return E::Uv0;
  if (iequal(str, "color"))
    return E::Color;
  if (iequal(str, "uv-region"))
    return E::UvRegion;
  if (iequal(str, "feature-index"))
    return E::FeatureIndex;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::GeometrySchema::AttributeField>
enumFromString<CesiumI3S::GeometrySchema::AttributeField>(
    const std::string_view& str) {
  using E = CesiumI3S::GeometrySchema::AttributeField;
  if (iequal(str, "position"))
    return E::Position;
  if (iequal(str, "normal"))
    return E::Normal;
  if (iequal(str, "uv0"))
    return E::Uv0;
  if (iequal(str, "color"))
    return E::Color;
  if (iequal(str, "region"))
    return E::Region;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::GeometryTopology>
enumFromString<CesiumI3S::GeometryTopology>(const std::string_view& str) {
  using E = CesiumI3S::GeometryTopology;
  if (iequal(str, "triangle") || iequal(str, "triangles"))
    return E::Triangle;
  if (iequal(str, "point") || iequal(str, "points"))
    return E::Point;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Geometry::Storage>
enumFromString<CesiumI3S::Geometry::Storage>(const std::string_view& str) {
  using E = CesiumI3S::Geometry::Storage;
  if (iequal(str, "arraybufferview"))
    return E::ArrayBufferView;
  if (iequal(str, "geometryreference"))
    return E::GeometryReference;
  if (iequal(str, "sharedresourcereference"))
    return E::SharedResourceReference;
  if (iequal(str, "embedded"))
    return E::Embedded;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Sublayer::Discipline>
enumFromString<CesiumI3S::Sublayer::Discipline>(const std::string_view& str) {
  using E = CesiumI3S::Sublayer::Discipline;
  if (iequal(str, "mechanical"))
    return E::Mechanical;
  if (iequal(str, "architectural"))
    return E::Architectural;
  if (iequal(str, "piping"))
    return E::Piping;
  if (iequal(str, "electrical"))
    return E::Electrical;
  if (iequal(str, "structural"))
    return E::Structural;
  if (iequal(str, "infrastructure"))
    return E::Infrastructure;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::Sublayer::Type>
enumFromString<CesiumI3S::Sublayer::Type>(const std::string_view& str) {
  using E = CesiumI3S::Sublayer::Type;
  if (iequal(str, "group"))
    return E::Group;
  if (iequal(str, "3dobject"))
    return E::ThreeDObject;
  if (iequal(str, "point"))
    return E::Point;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::FilterMode::Type>
enumFromString<CesiumI3S::FilterMode::Type>(const std::string_view& str) {
  using E = CesiumI3S::FilterMode::Type;
  if (iequal(str, "solid"))
    return E::Solid;
  if (iequal(str, "wireframe"))
    return E::WireFrame;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::AttributeStats::ModelName>
enumFromString<CesiumI3S::AttributeStats::ModelName>(
    const std::string_view& str) {
  using E = CesiumI3S::AttributeStats::ModelName;
  if (iequal(str, "category"))
    return E::Category;
  if (iequal(str, "family"))
    return E::Family;
  if (iequal(str, "familytype"))
    return E::FamilyType;
  if (iequal(str, "bldglevel"))
    return E::BldgLevel;
  if (iequal(str, "createdphase"))
    return E::CreatedPhase;
  if (iequal(str, "demolishedphase"))
    return E::DemolishedPhase;
  if (iequal(str, "discipline"))
    return E::Discipline;
  if (iequal(str, "assemblycode"))
    return E::AssemblyCode;
  if (iequal(str, "omniclass"))
    return E::OmniClass;
  if (iequal(str, "systemclassifications"))
    return E::SystemClassifications;
  if (iequal(str, "systemtype"))
    return E::SystemType;
  if (iequal(str, "systemname"))
    return E::SystemName;
  if (iequal(str, "systemclass"))
    return E::SystemClass;
  if (iequal(str, "custom"))
    return E::Custom;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::BoundingVolume>
enumFromString<CesiumI3S::BoundingVolume>(const std::string_view& str) {
  using E = CesiumI3S::BoundingVolume;
  if (iequal(str, "obb") || iequal(str, "orientedboundingbox"))
    return E::OrientedBoundingBox;
  if (iequal(str, "mbs") || iequal(str, "minimumboundingsphere"))
    return E::MinimumBoundingSphere;
  return std::nullopt;
}

template <>
std::optional<CesiumI3S::AttributeStorageInfo::Encoding>
enumFromString<CesiumI3S::AttributeStorageInfo::Encoding>(
    const std::string_view& str) {
  using E = CesiumI3S::AttributeStorageInfo::Encoding;
  if (iequal(str, "embedded-elevation"))
    return E::EmbeddedElevation;
  if (iequal(str, "lepcc-intensity"))
    return E::LepccIntensity;
  if (iequal(str, "lepcc-rgb"))
    return E::LepccRgb;
  return std::nullopt;
}

} // namespace CesiumI3SReader
