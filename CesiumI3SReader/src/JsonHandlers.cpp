#include "AttributeStorageJsonHandlers.h"
#include "BuildingLayerJsonHandlers.h"
#include "DefaultGeometrySchemaJsonHandlers.h"
#include "DrawingInfoJsonHandlers.h"
#include "FeatureDataJsonHandler.h"
#include "FieldsJsonHandlers.h"
#include "GeometryJsonHandlers.h"
#include "IEqual.h"
#include "LodSelectionJsonHandler.h"
#include "MaterialJsonHandlers.h"
#include "MeshJsonHandlers.h"
#include "NodeIndexDocumentJsonHandler.h"
#include "NodeJsonHandlers.h"
#include "PointCloudJsonHandlers.h"
#include "ResourceJsonHandler.h"
#include "SceneLayerJsonHandler.h"
#include "SpatialReferenceJsonHandlers.h"
#include "StatsJsonHandlers.h"
#include "StoreJsonHandler.h"
#include "TextureDefinitionJsonHandlers.h"

#include <CesiumUtility/Assert.h>

using namespace CesiumJsonReader;

namespace CesiumI3SReader {

SpatialReferenceJsonHandler::SpatialReferenceJsonHandler() noexcept = default;

void SpatialReferenceJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::SpatialReference* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
SpatialReferenceJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "latestVcsWkid")
    return property("latestVcsWkid", _latestVcsWkid, o.latestVcsWkid);
  if (str == "latestWkid")
    return property("latestWkid", _latestWkid, o.latestWkid);
  if (str == "vcsWkid")
    return property("vcsWkid", _vcsWkid, o.vcsWkid);
  if (str == "wkid")
    return property("wkid", _wkid, o.wkid);
  if (str == "wkt")
    return property("wkt", _wkt, o.wkt);
  return this->ignoreAndContinue();
}
HeightModelInfoJsonHandler::HeightModelInfoJsonHandler() noexcept = default;

void HeightModelInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::HeightModelInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
HeightModelInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "heightModel")
    return property("heightModel", _heightModel, o.heightModel);
  if (iequal(str, "vertCRS"))
    return property("vertCrs", _vertCrs, o.vertCrs);
  if (str == "heightUnit")
    return property("heightUnit", _heightUnit, o.heightUnit);
  return this->ignoreAndContinue();
}

ObbJsonHandler::ObbJsonHandler() noexcept = default;

void ObbJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::OrientedBoundingBox* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* ObbJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "center")
    return property("center", _center, o.center);
  if (str == "halfSize")
    return property("halfSize", _halfSize, o.halfSize);
  if (str == "quaternion")
    return property("quaternion", _quaternion, o.quaternion);
  return this->ignoreAndContinue();
}

MinimumBoundingSphereJsonHandler::MinimumBoundingSphereJsonHandler() noexcept =
    default;

void MinimumBoundingSphereJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MinimumBoundingSphere* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MinimumBoundingSphereJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "center")
    return property("center", _center, o.center);
  if (str == "radius")
    return property("radius", _radius, o.radius);
  return this->ignoreAndContinue();
}

ElevationInfoJsonHandler::ElevationInfoJsonHandler() noexcept = default;

void ElevationInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::ElevationInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
ElevationInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "mode")
    return property("mode", _mode, o.mode);
  if (str == "offset")
    return property("offset", _offset, o.offset);
  if (str == "unit")
    return property("unit", _unit, o.unit);
  return this->ignoreAndContinue();
}

FullExtentJsonHandler::FullExtentJsonHandler() noexcept = default;

void FullExtentJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::FullExtent* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
FullExtentJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "xmin")
    return property("xmin", _xmin, o.xmin);
  if (str == "ymin")
    return property("ymin", _ymin, o.ymin);
  if (str == "xmax")
    return property("xmax", _xmax, o.xmax);
  if (str == "ymax")
    return property("ymax", _ymax, o.ymax);
  if (str == "zmin")
    return property("zmin", _zmin, o.zmin);
  if (str == "zmax")
    return property("zmax", _zmax, o.zmax);
  if (str == "spatialReference")
    return property("spatialReference", _spatialReference, o.spatialReference);
  return this->ignoreAndContinue();
}

ExtentJsonHandler::ExtentJsonHandler() noexcept = default;

void ExtentJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Extent* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* ExtentJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "xmin")
    return property("xmin", _xmin, o.xmin);
  if (str == "ymin")
    return property("ymin", _ymin, o.ymin);
  if (str == "xmax")
    return property("xmax", _xmax, o.xmax);
  if (str == "ymax")
    return property("ymax", _ymax, o.ymax);
  return this->ignoreAndContinue();
}

HeaderAttributeJsonHandler::HeaderAttributeJsonHandler() noexcept = default;

void HeaderAttributeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::HeaderAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
HeaderAttributeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "property")
    return property("property", _property, o.property);
  if (str == "type")
    return property("type", _type, o.type);
  return this->ignoreAndContinue();
}

HeaderValueJsonHandler::HeaderValueJsonHandler() noexcept = default;

void HeaderValueJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::HeaderValue* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
HeaderValueJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "valueType")
    return property("valueType", _valueType, o.valueType);
  if (str == "property")
    return property("property", _property, o.property);
  return this->ignoreAndContinue();
}

ValueJsonHandler::ValueJsonHandler() noexcept = default;

void ValueJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Value* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* ValueJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "valueType")
    return property("valueType", _valueType, o.valueType);
  if (str == "encoding")
    return property("encoding", _encoding, o.encoding);
  if (str == "timeEncoding")
    return property("timeEncoding", _timeEncoding, o.timeEncoding);
  if (str == "valuesPerElement")
    return property("valuesPerElement", _valuesPerElement, o.valuesPerElement);
  return this->ignoreAndContinue();
}

AttributeStorageInfoJsonHandler::AttributeStorageInfoJsonHandler() noexcept =
    default;

void AttributeStorageInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::AttributeStorageInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
AttributeStorageInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "key")
    return property("key", _key, o.key);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "header")
    return property("header", _header, o.header);
  if (str == "ordering")
    return property("ordering", _ordering, o.ordering);
  if (str == "attributeValues")
    return property("attributeValues", _attributeValues, o.attributeValues);
  if (str == "attributeByteCounts")
    return property(
        "attributeByteCounts",
        _attributeByteCounts,
        o.attributeByteCounts);
  if (str == "objectIds")
    return property("objectIds", _objectIds, o.objectIds);
  if (str == "encoding")
    return property("encoding", _encoding, o.encoding);
  return this->ignoreAndContinue();
}

FeatureAttributeJsonHandler::FeatureAttributeJsonHandler() noexcept = default;

void FeatureAttributeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::FeatureAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
FeatureAttributeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "faceRange")
    return property("faceRange", _faceRange, o.faceRange);
  return this->ignoreAndContinue();
}

RendererEdgesJsonHandler::RendererEdgesJsonHandler() noexcept = default;

void RendererEdgesJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::RendererEdges* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
RendererEdgesJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "color")
    return property("color", _color, _pObject->color);
  if (str == "transparency")
    return property("transparency", _transparency, _pObject->transparency);
  return this->ignoreAndContinue();
}

RendererMaterialJsonHandler::RendererMaterialJsonHandler() noexcept = default;

void RendererMaterialJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::RendererMaterial* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
RendererMaterialJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "colorMixMode")
    return property("colorMixMode", _colorMixMode, _pObject->colorMixMode);
  if (str == "color")
    return property("color", _color, _pObject->color);
  if (str == "transparency")
    return property("transparency", _transparency, _pObject->transparency);
  return this->ignoreAndContinue();
}

SymbolLayerJsonHandler::SymbolLayerJsonHandler() noexcept = default;

void SymbolLayerJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::SymbolLayer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
SymbolLayerJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "type")
    return property("type", _type, _pObject->type);
  if (str == "edges")
    return property("edges", _edges, _pObject->edges);
  if (str == "outline")
    return property("outline", _outline, _pObject->outline);
  if (str == "material")
    return property("material", _material, _pObject->material);
  return this->ignoreAndContinue();
}

SymbolJsonHandler::SymbolJsonHandler() noexcept = default;

void SymbolJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Symbol* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* SymbolJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "symbolLayers")
    return property("symbolLayers", _symbolLayers, _pObject->symbolLayers);
  return this->ignoreAndContinue();
}

UniqueValueInfoJsonHandler::UniqueValueInfoJsonHandler() noexcept = default;

void UniqueValueInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::UniqueValueInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
UniqueValueInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "value")
    return property("value", _value, _pObject->value);
  if (str == "symbol")
    return property("symbol", _symbol, _pObject->symbol);
  return this->ignoreAndContinue();
}

UniqueValueClassJsonHandler::UniqueValueClassJsonHandler() noexcept = default;

void UniqueValueClassJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::UniqueValueClass* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
UniqueValueClassJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "symbol")
    return property("symbol", _symbol, _pObject->symbol);
  if (str == "values")
    return property("values", _values, _pObject->values);
  return this->ignoreAndContinue();
}

UniqueValueGroupJsonHandler::UniqueValueGroupJsonHandler() noexcept = default;

void UniqueValueGroupJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::UniqueValueGroup* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
UniqueValueGroupJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "classes")
    return property("classes", _classes, _pObject->classes);
  return this->ignoreAndContinue();
}

ClassBreakInfoJsonHandler::ClassBreakInfoJsonHandler() noexcept = default;

void ClassBreakInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::ClassBreakInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
ClassBreakInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "classMinValue")
    return property("classMinValue", _classMinValue, _pObject->classMinValue);
  if (str == "classMaxValue")
    return property("classMaxValue", _classMaxValue, _pObject->classMaxValue);
  if (str == "symbol")
    return property("symbol", _symbol, _pObject->symbol);
  return this->ignoreAndContinue();
}

RendererJsonHandler::RendererJsonHandler() noexcept = default;

void RendererJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Renderer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* RendererJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "type")
    return property("type", _type, o.type);
  if (str == "symbol")
    return property("symbol", _symbol, o.symbol);
  if (str == "defaultSymbol")
    return property("defaultSymbol", _defaultSymbol, o.defaultSymbol);
  if (str == "field1")
    return property("field1", _field1, o.field1);
  if (str == "field2")
    return property("field2", _field2, o.field2);
  if (str == "field3")
    return property("field3", _field3, o.field3);
  if (str == "field")
    return property("field", _field, o.field);
  if (str == "minValue")
    return property("minValue", _minValue, o.minValue);
  if (str == "uniqueValueInfos")
    return property("uniqueValueInfos", _uniqueValueInfos, o.uniqueValueInfos);
  if (str == "uniqueValueGroups")
    return property(
        "uniqueValueGroups",
        _uniqueValueGroups,
        o.uniqueValueGroups);
  if (str == "classBreakInfos")
    return property("classBreakInfos", _classBreakInfos, o.classBreakInfos);
  return this->ignoreAndContinue();
}

CachedDrawingInfoJsonHandler::CachedDrawingInfoJsonHandler() noexcept = default;

void CachedDrawingInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::CachedDrawingInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
CachedDrawingInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "color")
    return property("color", _color, _pObject->color);
  return this->ignoreAndContinue();
}

DrawingInfoJsonHandler::DrawingInfoJsonHandler() noexcept = default;

void DrawingInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::DrawingInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
DrawingInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "renderer")
    return property("renderer", _renderer, _pObject->renderer);
  if (str == "scaleSymbols")
    return property("scaleSymbols", _scaleSymbols, _pObject->scaleSymbols);
  return this->ignoreAndContinue();
}

ServiceUpdateTimeStampJsonHandler::
    ServiceUpdateTimeStampJsonHandler() noexcept = default;

void ServiceUpdateTimeStampJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::ServiceUpdateTimeStamp* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
ServiceUpdateTimeStampJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "lastUpdate")
    return property("lastUpdate", _lastUpdate, _pObject->lastUpdate);
  return this->ignoreAndContinue();
}

ValueCountJsonHandler::ValueCountJsonHandler() noexcept = default;

void ValueCountJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::ValueCount* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
ValueCountJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "value")
    return property("value", _value, o.value);
  if (str == "count")
    return property("count", _count, o.count);
  return this->ignoreAndContinue();
}

HistogramJsonHandler::HistogramJsonHandler() noexcept = default;

void HistogramJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Histogram* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* HistogramJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "minimum")
    return property("minimum", _minimum, o.minimum);
  if (str == "maximum")
    return property("maximum", _maximum, o.maximum);
  if (str == "counts")
    return property("counts", _counts, o.counts);
  return this->ignoreAndContinue();
}

StatsInfoJsonHandler::StatsInfoJsonHandler() noexcept = default;

void StatsInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::StatsInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* StatsInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "totalValuesCount")
    return property("totalValuesCount", _totalValuesCount, o.totalValuesCount);
  if (str == "min")
    return property("min", _min, o.min);
  if (str == "max")
    return property("max", _max, o.max);
  if (str == "minTimeStr")
    return property("minTimeStr", _minTimeStr, o.minTimeStr);
  if (str == "maxTimeStr")
    return property("maxTimeStr", _maxTimeStr, o.maxTimeStr);
  if (str == "count")
    return property("count", _count, o.count);
  if (str == "sum")
    return property("sum", _sum, o.sum);
  if (str == "avg")
    return property("avg", _avg, o.avg);
  if (str == "stddev")
    return property("stddev", _stddev, o.stddev);
  if (str == "variance")
    return property("variance", _variance, o.variance);
  if (str == "histogram")
    return property("histogram", _histogram, o.histogram);
  if (str == "mostFrequentValues")
    return property(
        "mostFrequentValues",
        _mostFrequentValues,
        o.mostFrequentValues);
  return this->ignoreAndContinue();
}

StatisticsJsonHandler::StatisticsJsonHandler() noexcept = default;

void StatisticsJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Statistics* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
StatisticsJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "stats")
    return property("stats", _stats, _pObject->stats);
  return this->ignoreAndContinue();
}

StatisticsInfoJsonHandler::StatisticsInfoJsonHandler() noexcept = default;

void StatisticsInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::StatisticsInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
StatisticsInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "key")
    return property("key", _key, o.key);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "href")
    return property("href", _href, o.href);
  return this->ignoreAndContinue();
}

DomainCodedValueJsonHandler::DomainCodedValueJsonHandler() noexcept = default;

void DomainCodedValueJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::DomainCodedValue* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
DomainCodedValueJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "code")
    return property("code", _code, o.code);
  return this->ignoreAndContinue();
}

DomainJsonHandler::DomainJsonHandler() noexcept = default;

void DomainJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Domain* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* DomainJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "type")
    return property("type", _type, o.type);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "description")
    return property("description", _description, o.description);
  if (str == "fieldType")
    return property("fieldType", _fieldType, o.fieldType);
  if (str == "range")
    return property("range", _range, o.range);
  if (str == "codedValues")
    return property("codedValues", _codedValues, o.codedValues);
  if (str == "mergePolicy")
    return property("mergePolicy", _mergePolicy, o.mergePolicy);
  if (str == "splitPolicy")
    return property("splitPolicy", _splitPolicy, o.splitPolicy);
  return this->ignoreAndContinue();
}

FieldJsonHandler::FieldJsonHandler() noexcept = default;

void FieldJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Field* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* FieldJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "type")
    return property("type", _type, o.type);
  if (str == "alias")
    return property("alias", _alias, o.alias);
  if (str == "domain")
    return property("domain", _domain, o.domain);
  return this->ignoreAndContinue();
}

ResourceJsonHandler::ResourceJsonHandler() noexcept = default;

void ResourceJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Resource* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* ResourceJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "href")
    return property("href", _href, o.href);
  if (str == "layerContent")
    return property("layerContent", _layerContent, o.layerContent);
  if (str == "featureRange")
    return property("featureRange", _featureRange, o.featureRange);
  if (str == "multiTextureBundle")
    return property(
        "multiTextureBundle",
        _multiTextureBundle,
        o.multiTextureBundle);
  if (str == "vertexElements")
    return property("vertexElements", _vertexElements, o.vertexElements);
  if (str == "faceElements")
    return property("faceElements", _faceElements, o.faceElements);
  return this->ignoreAndContinue();
}

LodSelectionJsonHandler::LodSelectionJsonHandler() noexcept = default;

void LodSelectionJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::LodSelection* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
LodSelectionJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "metricType")
    return property("metricType", _metricType, o.metricType);
  if (str == "maxError")
    return property("maxError", _maxError, o.maxError);
  return this->ignoreAndContinue();
}

MeshMaterialJsonHandler::MeshMaterialJsonHandler() noexcept = default;

void MeshMaterialJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MeshMaterial* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MeshMaterialJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "definition")
    return property("definition", _definition, o.definition);
  if (str == "resource")
    return property("resource", _resource, o.resource);
  if (str == "texelCountHint")
    return property("texelCountHint", _texelCountHint, o.texelCountHint);
  return this->ignoreAndContinue();
}

MeshGeometryJsonHandler::MeshGeometryJsonHandler() noexcept = default;

void MeshGeometryJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MeshGeometry* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MeshGeometryJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "definition")
    return property("definition", _definition, o.definition);
  if (str == "resource")
    return property("resource", _resource, o.resource);
  if (str == "vertexCount")
    return property("vertexCount", _vertexCount, o.vertexCount);
  if (str == "featureCount")
    return property("featureCount", _featureCount, o.featureCount);
  return this->ignoreAndContinue();
}

MeshAttributeJsonHandler::MeshAttributeJsonHandler() noexcept = default;

void MeshAttributeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MeshAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MeshAttributeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "resource")
    return property("resource", _resource, _pObject->resource);
  return this->ignoreAndContinue();
}

MeshJsonHandler::MeshJsonHandler() noexcept = default;

void MeshJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Mesh* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* MeshJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "geometry")
    return property("geometry", _geometry, o.geometry);
  if (str == "material")
    return property("material", _material, o.material);
  if (str == "attribute")
    return property("attribute", _attribute, o.attribute);
  return this->ignoreAndContinue();
}

TextureSlotReferenceJsonHandler::TextureSlotReferenceJsonHandler() noexcept =
    default;

void TextureSlotReferenceJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::TextureSlotReference* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
TextureSlotReferenceJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "textureSetDefinitionId")
    return property(
        "textureSetDefinitionId",
        _textureSetDefinitionId,
        o.textureSetDefinitionId);
  if (str == "texCoord")
    return property("texCoord", _texCoord, o.texCoord);
  if (str == "factor")
    return property("factor", _factor, o.factor);
  return this->ignoreAndContinue();
}

PbrMetallicRoughnessJsonHandler::PbrMetallicRoughnessJsonHandler() noexcept =
    default;

void PbrMetallicRoughnessJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PbrMetallicRoughness* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
PbrMetallicRoughnessJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "baseColorFactor")
    return property("baseColorFactor", _baseColorFactor, o.baseColorFactor);
  if (str == "baseColorTexture")
    return property("baseColorTexture", _baseColorTexture, o.baseColorTexture);
  if (str == "metallicFactor")
    return property("metallicFactor", _metallicFactor, o.metallicFactor);
  if (str == "roughnessFactor")
    return property("roughnessFactor", _roughnessFactor, o.roughnessFactor);
  if (str == "metallicRoughnessTexture")
    return property(
        "metallicRoughnessTexture",
        _metallicRoughnessTexture,
        o.metallicRoughnessTexture);
  return this->ignoreAndContinue();
}

MaterialDefinitionJsonHandler::MaterialDefinitionJsonHandler() noexcept =
    default;

void MaterialDefinitionJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MaterialDefinition* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MaterialDefinitionJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "pbrMetallicRoughness")
    return property(
        "pbrMetallicRoughness",
        _pbrMetallicRoughness,
        o.pbrMetallicRoughness);
  if (str == "normalTexture")
    return property("normalTexture", _normalTexture, o.normalTexture);
  if (str == "occlusionTexture")
    return property("occlusionTexture", _occlusionTexture, o.occlusionTexture);
  if (str == "emissiveTexture")
    return property("emissiveTexture", _emissiveTexture, o.emissiveTexture);
  if (str == "emissiveFactor")
    return property("emissiveFactor", _emissiveFactor, o.emissiveFactor);
  if (str == "alphaMode")
    return property("alphaMode", _alphaMode, o.alphaMode);
  if (str == "alphaCutoff")
    return property("alphaCutoff", _alphaCutoff, o.alphaCutoff);
  if (str == "doubleSided")
    return property("doubleSided", _doubleSided, o.doubleSided);
  if (str == "cullFace")
    return property("cullFace", _cullFace, o.cullFace);
  return this->ignoreAndContinue();
}

MaterialParamsJsonHandler::MaterialParamsJsonHandler() noexcept = default;

void MaterialParamsJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MaterialParams* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MaterialParamsJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "transparency")
    return property("transparency", _transparency, o.transparency);
  if (str == "reflectivity")
    return property("reflectivity", _reflectivity, o.reflectivity);
  if (str == "shininess")
    return property("shininess", _shininess, o.shininess);
  if (str == "ambient")
    return property("ambient", _ambient, o.ambient);
  if (str == "diffuse")
    return property("diffuse", _diffuse, o.diffuse);
  if (str == "specular")
    return property("specular", _specular, o.specular);
  if (str == "renderMode")
    return property("renderMode", _renderMode, o.renderMode);
  if (str == "castShadows")
    return property("castShadows", _castShadows, o.castShadows);
  if (str == "receiveShadows")
    return property("receiveShadows", _receiveShadows, o.receiveShadows);
  if (str == "cullFace")
    return property("cullFace", _cullFace, o.cullFace);
  if (str == "vertexColors")
    return property("vertexColors", _vertexColors, o.vertexColors);
  if (str == "vertexRegions")
    return property("vertexRegions", _vertexRegions, o.vertexRegions);
  if (str == "useVertexColorAlpha")
    return property(
        "useVertexColorAlpha",
        _useVertexColorAlpha,
        o.useVertexColorAlpha);
  return this->ignoreAndContinue();
}

MaterialDefinitionInfoJsonHandler::
    MaterialDefinitionInfoJsonHandler() noexcept = default;

void MaterialDefinitionInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::MaterialDefinitionInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
MaterialDefinitionInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "type")
    return property("type", _type, o.type);
  if (str == "sharedResourceHref")
    return property(
        "sharedResourceHref",
        _sharedResourceHref,
        o.sharedResourceHref);
  if (str == "params")
    return property("params", _params, o.params);
  return this->ignoreAndContinue();
}

ImageJsonHandler::ImageJsonHandler() noexcept = default;

void ImageJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Image* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* ImageJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "size")
    return property("size", _size, o.size);
  if (str == "pixelInWorldUnits")
    return property(
        "pixelInWorldUnits",
        _pixelInWorldUnits,
        o.pixelInWorldUnits);
  if (str == "href")
    return property("href", _href, o.href);
  if (str == "byteOffset")
    return property("byteOffset", _byteOffset, o.byteOffset);
  if (str == "length")
    return property("length", _length, o.length);
  return this->ignoreAndContinue();
}

TextureDefinitionInfoJsonHandler::TextureDefinitionInfoJsonHandler() noexcept =
    default;

void TextureDefinitionInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::TextureDefinitionInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
TextureDefinitionInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "encoding")
    return property("encoding", _encoding, o.encoding);
  if (str == "wrap")
    return property("wrap", _wrap, o.wrap);
  if (str == "atlas")
    return property("atlas", _atlas, o.atlas);
  if (str == "uvSet")
    return property("uvSet", _uvSet, o.uvSet);
  if (str == "channels")
    return property("channels", _channels, o.channels);
  if (str == "images")
    return property("images", _images, o.images);
  return this->ignoreAndContinue();
}

TextureSetDefinitionFormatJsonHandler::
    TextureSetDefinitionFormatJsonHandler() noexcept = default;

void TextureSetDefinitionFormatJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::TextureSetDefinitionFormat* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* TextureSetDefinitionFormatJsonHandler::readObjectKey(
    const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "format")
    return property("format", _format, o.format);
  return this->ignoreAndContinue();
}

TextureSetDefinitionJsonHandler::TextureSetDefinitionJsonHandler() noexcept =
    default;

void TextureSetDefinitionJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::TextureSetDefinition* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
TextureSetDefinitionJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "formats")
    return property("formats", _formats, o.formats);
  if (str == "atlas")
    return property("atlas", _atlas, o.atlas);
  return this->ignoreAndContinue();
}

GeometryAttributeJsonHandler::GeometryAttributeJsonHandler() noexcept = default;

void GeometryAttributeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::GeometryAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
GeometryAttributeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "byteOffset")
    return property("byteOffset", _byteOffset, o.byteOffset);
  if (str == "valueType")
    return property("valueType", _valueType, o.valueType);
  if (str == "valuesPerElement")
    return property("valuesPerElement", _valuesPerElement, o.valuesPerElement);
  return this->ignoreAndContinue();
}

VertexAttributeJsonHandler::VertexAttributeJsonHandler() noexcept = default;

void VertexAttributeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::VertexAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
VertexAttributeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "position")
    return property("position", _position, o.position);
  if (str == "normal")
    return property("normal", _normal, o.normal);
  if (str == "uv0")
    return property("uv0", _uv0, o.uv0);
  if (str == "color")
    return property("color", _color, o.color);
  if (str == "region")
    return property("region", _region, o.region);
  return this->ignoreAndContinue();
}

GeometrySchemaJsonHandler::GeometrySchemaJsonHandler() noexcept = default;

void GeometrySchemaJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::GeometrySchema* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
GeometrySchemaJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "geometryType")
    return property("geometryType", _geometryType, o.geometryType);
  if (str == "topology")
    return property("topology", _topology, o.topology);
  if (str == "header")
    return property("header", _header, o.header);
  if (str == "ordering")
    return property("ordering", _ordering, o.ordering);
  if (str == "vertexAttributes")
    return property("vertexAttributes", _vertexAttributes, o.vertexAttributes);
  if (str == "faces")
    return property("faces", _faces, o.faces);
  if (str == "featureAttributeOrder")
    return property(
        "featureAttributeOrder",
        _featureAttributeOrder,
        o.featureAttributeOrder);
  if (str == "featureAttributes")
    return property(
        "featureAttributes",
        _featureAttributes,
        o.featureAttributes);
  return this->ignoreAndContinue();
}

CompressedAttributesJsonHandler::CompressedAttributesJsonHandler() noexcept =
    default;

void CompressedAttributesJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Compression* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
CompressedAttributesJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "encoding")
    return property("encoding", _encoding, o.encoding);
  if (str == "attributes")
    return property("attributes", _attributes, o.attributes);
  return this->ignoreAndContinue();
}

GeometryAttributeDescJsonHandler::GeometryAttributeDescJsonHandler() noexcept =
    default;

void GeometryAttributeDescJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::GeometryBuffer::Attribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
GeometryAttributeDescJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "component")
    return property("component", _component, _pObject->component);
  if (str == "binding")
    return property("binding", _binding, _pObject->binding);
  return this->ignoreAndContinue();
}

FeatureIdAttributeDescJsonHandler::
    FeatureIdAttributeDescJsonHandler() noexcept = default;

void FeatureIdAttributeDescJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::GeometryBuffer::FeatureIdAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
FeatureIdAttributeDescJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "component")
    return property("component", _component, _pObject->component);
  if (str == "binding")
    return property("binding", _binding, _pObject->binding);
  if (str == "type")
    return property("type", _type, _pObject->type);
  return this->ignoreAndContinue();
}

GeometryBufferJsonHandler::GeometryBufferJsonHandler() noexcept = default;

void GeometryBufferJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::GeometryBuffer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
GeometryBufferJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "offset")
    return property("offset", _offset, o.offset);
  if (str == "position")
    return property("position", _position, o.position);
  if (str == "normal")
    return property("normal", _normal, o.normal);
  if (str == "uv0")
    return property("uv0", _uv0, o.uv0);
  if (str == "color")
    return property("color", _color, o.color);
  if (str == "uvRegion")
    return property("uvRegion", _uvRegion, o.uvRegion);
  if (str == "featureId")
    return property("featureId", _featureId, o.featureId);
  if (str == "faceRange")
    return property("faceRange", _faceRange, o.faceRange);
  if (str == "compressedAttributes")
    return property(
        "compressedAttributes",
        _compressedAttributes,
        o.compression);
  return this->ignoreAndContinue();
}

GeometryDefinitionJsonHandler::GeometryDefinitionJsonHandler() noexcept =
    default;

void GeometryDefinitionJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::GeometryDefinition* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
GeometryDefinitionJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "topology")
    return property("topology", _topology, o.topology);
  if (str == "geometryBuffers")
    return property("geometryBuffers", _geometryBuffers, o.geometryBuffers);
  return this->ignoreAndContinue();
}

GeometryJsonHandler::GeometryJsonHandler() noexcept = default;

void GeometryJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Geometry* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* GeometryJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "type")
    return property("type", _type, o.type);
  if (str == "transformation")
    return property("transformation", _transformation, o.transformation);
  // "params" is raw JSON — skip.
  return this->ignoreAndContinue();
}

StoreJsonHandler::StoreJsonHandler() noexcept = default;

void StoreJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Store* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* StoreJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "profile")
    return property("profile", _profile, o.profile);
  if (str == "resourcePattern")
    return property("resourcePattern", _resourcePattern, o.resourcePattern);
  if (str == "rootNode")
    return property("rootNode", _rootNode, o.rootNode);
  if (str == "version")
    return property("version", _version, o.version);
  if (str == "extent")
    return property("extent", _extent, o.extent);
  if (str == "indexCrs")
    return property("indexCrs", _indexCrs, o.indexCrs);
  if (str == "vertexCrs")
    return property("vertexCrs", _vertexCrs, o.vertexCrs);
  if (str == "normalReferenceFrame")
    return property(
        "normalReferenceFrame",
        _normalReferenceFrame,
        o.normalReferenceFrame);
  if (str == "nidEncoding")
    return property("nidEncoding", _nidEncoding, o.nidEncoding);
  if (str == "featureEncoding")
    return property("featureEncoding", _featureEncoding, o.featureEncoding);
  if (str == "geometryEncoding")
    return property("geometryEncoding", _geometryEncoding, o.geometryEncoding);
  if (str == "attributeEncoding")
    return property(
        "attributeEncoding",
        _attributeEncoding,
        o.attributeEncoding);
  if (str == "textureEncoding")
    return property("textureEncoding", _textureEncoding, o.textureEncoding);
  if (str == "lodType")
    return property("lodType", _lodType, o.lodType);
  if (str == "lodModel")
    return property("lodModel", _lodModel, o.lodModel);
  if (str == "indexingScheme")
    return property("indexingScheme", _indexingScheme, o.indexingScheme);
  if (str == "defaultGeometrySchema")
    return property(
        "defaultGeometrySchema",
        _defaultGeometrySchema,
        o.defaultGeometrySchema);
  if (str == "defaultTextureDefinition")
    return property(
        "defaultTextureDefinition",
        _defaultTextureDefinition,
        o.defaultTextureDefinition);
  if (str == "defaultMaterialDefinition")
    return property(
        "defaultMaterialDefinition",
        _defaultMaterialDefinition,
        o.defaultMaterialDefinition);
  return this->ignoreAndContinue();
}

NodePageDefinitionJsonHandler::NodePageDefinitionJsonHandler() noexcept =
    default;

void NodePageDefinitionJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::NodePageDefinition* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
NodePageDefinitionJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "nodesPerPage")
    return property("nodesPerPage", _nodesPerPage, o.nodesPerPage);
  if (str == "rootIndex")
    return property("rootIndex", _rootIndex, o.rootIndex);
  if (str == "lodSelectionMetricType")
    return property(
        "lodSelectionMetricType",
        _lodSelectionMetricType,
        o.lodSelectionMetricType);
  return this->ignoreAndContinue();
}

NodeReferenceJsonHandler::NodeReferenceJsonHandler() noexcept = default;

void NodeReferenceJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::NodeReference* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
NodeReferenceJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "mbs")
    return property("mbs", _mbs, o.mbs);
  if (str == "href")
    return property("href", _href, o.href);
  if (str == "version")
    return property("version", _version, o.version);
  if (str == "featureCount")
    return property("featureCount", _featureCount, o.featureCount);
  if (str == "obb")
    return property("obb", _obb, o.obb);
  return this->ignoreAndContinue();
}

NodeJsonHandler::NodeJsonHandler() noexcept = default;

void NodeJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Node* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* NodeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "index")
    return property("index", _index, o.index);
  if (str == "parentIndex")
    return property("parentIndex", _parentIndex, o.parentIndex);
  if (str == "lodThreshold")
    return property("lodThreshold", _lodThreshold, o.lodThreshold);
  if (str == "obb")
    return property("obb", _obb, o.obb);
  if (str == "children")
    return property("children", _children, o.children);
  if (str == "mesh")
    return property("mesh", _mesh, o.mesh);
  return this->ignoreAndContinue();
}

NodePageJsonHandler::NodePageJsonHandler() noexcept = default;

void NodePageJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::NodePage* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* NodePageJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "nodes")
    return property("nodes", _nodes, _pObject->nodes);
  return this->ignoreAndContinue();
}

FeatureDataJsonHandler::FeatureDataJsonHandler() noexcept = default;

void FeatureDataJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::FeatureData* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
FeatureDataJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "position")
    return property("position", _position, o.position);
  if (str == "pivotOffset")
    return property("pivotOffset", _pivotOffset, o.pivotOffset);
  if (str == "mbb")
    return property("mbb", _mbb, o.mbb);
  if (str == "layer")
    return property("layer", _layer, o.layer);
  if (str == "attributes")
    return property("attributes", _attributes, o.attributes);
  if (str == "geometries")
    return property("geometries", _geometries, o.geometries);
  return this->ignoreAndContinue();
}

NodeIndexDocumentJsonHandler::NodeIndexDocumentJsonHandler() noexcept = default;

void NodeIndexDocumentJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::NodeIndexDocument* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
NodeIndexDocumentJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "level")
    return property("level", _level, o.level);
  if (str == "version")
    return property("version", _version, o.version);
  if (str == "mbs")
    return property("mbs", _mbs, o.mbs);
  if (str == "obb")
    return property("obb", _obb, o.obb);
  if (str == "transform")
    return property("transform", _transform, o.transform);
  if (str == "parentNode")
    return property("parentNode", _parentNode, o.parentNode);
  if (str == "children")
    return property("children", _children, o.children);
  if (str == "neighbors")
    return property("neighbors", _neighbors, o.neighbors);
  if (str == "sharedResource")
    return property("sharedResource", _sharedResource, o.sharedResource);
  if (str == "featureData")
    return property("featureData", _featureData, o.featureData);
  if (str == "geometryData")
    return property("geometryData", _geometryData, o.geometryData);
  if (str == "textureData")
    return property("textureData", _textureData, o.textureData);
  if (str == "attributeData")
    return property("attributeData", _attributeData, o.attributeData);
  if (str == "lodSelection")
    return property("lodSelection", _lodSelection, o.lodSelection);
  if (str == "features")
    return property("features", _features, o.features);
  return this->ignoreAndContinue();
}

TimeInfoJsonHandler::TimeInfoJsonHandler() noexcept = default;

void TimeInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::TimeInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* TimeInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "endTimeField")
    return property("endTimeField", _endTimeField, o.endTimeField);
  if (str == "startTimeField")
    return property("startTimeField", _startTimeField, o.startTimeField);
  return this->ignoreAndContinue();
}

RangeInfoJsonHandler::RangeInfoJsonHandler() noexcept = default;

void RangeInfoJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::RangeInfo* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* RangeInfoJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "field")
    return property("field", _field, o.field);
  if (str == "name")
    return property("name", _name, o.name);
  return this->ignoreAndContinue();
}

LayerJsonHandler::LayerJsonHandler() noexcept = default;

void LayerJsonHandler::reset(IJsonHandler* pParent, CesiumI3S::Layer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* LayerJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "href")
    return property("href", _href, o.href);
  if (str == "layerType")
    return property("layerType", _layerType, o.layerType);
  if (str == "spatialReference")
    return property("spatialReference", _spatialReference, o.spatialReference);
  if (str == "heightModelInfo")
    return property("heightModelInfo", _heightModelInfo, o.heightModelInfo);
  if (str == "version")
    return property("version", _version, o.version);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "alias")
    return property("alias", _alias, o.alias);
  if (str == "description")
    return property("description", _description, o.description);
  if (str == "copyrightText")
    return property("copyrightText", _copyrightText, o.copyrightText);
  if (str == "capabilities")
    return property("capabilities", _capabilities, o.capabilities);
  if (str == "cachedDrawingInfo")
    return property(
        "cachedDrawingInfo",
        _cachedDrawingInfo,
        o.cachedDrawingInfo);
  if (str == "drawingInfo")
    return property("drawingInfo", _drawingInfo, o.drawingInfo);
  if (str == "elevationInfo")
    return property("elevationInfo", _elevationInfo, o.elevationInfo);
  if (str == "serviceUpdateTimeStamp")
    return property(
        "serviceUpdateTimeStamp",
        _serviceUpdateTimeStamp,
        o.serviceUpdateTimeStamp);
  if (str == "store")
    return property("store", _store, o.store);
  if (str == "fields")
    return property("fields", _fields, o.fields);
  if (str == "attributeStorageInfo")
    return property(
        "attributeStorageInfo",
        _attributeStorageInfo,
        o.attributeStorageInfo);
  if (str == "statisticsInfo")
    return property("statisticsInfo", _statisticsInfo, o.statisticsInfo);
  if (str == "nodePages" || str == "pointNodePages")
    return property("nodePages", _nodePages, o.nodePages);
  if (str == "geometryDefinitions")
    return property(
        "geometryDefinitions",
        _geometryDefinitions,
        o.geometryDefinitions);
  if (str == "materialDefinitions")
    return property(
        "materialDefinitions",
        _materialDefinitions,
        o.materialDefinitions);
  if (str == "textureSetDefinitions")
    return property(
        "textureSetDefinitions",
        _textureSetDefinitions,
        o.textureSetDefinitions);
  if (str == "fullExtent")
    return property("fullExtent", _fullExtent, o.fullExtent);
  if (str == "zFactor")
    return property("zFactor", _zFactor, o.zFactor);
  if (str == "disablePopup")
    return property("disablePopup", _disablePopup, o.disablePopup);
  if (str == "timeInfo")
    return property("timeInfo", _timeInfo, o.timeInfo);
  if (str == "rangeInfo")
    return property("rangeInfo", _rangeInfo, o.rangeInfo);
  // "popupInfo" is raw JSON — skip.
  return this->ignoreAndContinue();
}

SublayerJsonHandler::SublayerJsonHandler() noexcept = default;

void SublayerJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Sublayer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* SublayerJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "alias")
    return property("alias", _alias, o.alias);
  if (str == "discipline")
    return property("discipline", _discipline, o.discipline);
  if (str == "modelName")
    return property("modelName", _modelName, o.modelName);
  if (str == "layerType")
    return property("layerType", _layerType, o.layerType);
  if (str == "visibility")
    return property("visibility", _visibility, o.visibility);
  if (str == "sublayers")
    return property("sublayers", _sublayers, o.sublayers);
  if (str == "isEmpty")
    return property("isEmpty", _isEmpty, o.isEmpty);
  return this->ignoreAndContinue();
}

FilterModeJsonHandler::FilterModeJsonHandler() noexcept = default;

void FilterModeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::FilterMode* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
FilterModeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "type")
    return property("type", _type, _pObject->type);
  // "edges" is raw JSON — skip.
  return this->ignoreAndContinue();
}

FilterBlockJsonHandler::FilterBlockJsonHandler() noexcept = default;

void FilterBlockJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::FilterBlock* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
FilterBlockJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "title")
    return property("title", _title, o.title);
  if (str == "filterMode")
    return property("filterMode", _filterMode, o.filterMode);
  if (str == "filterExpression")
    return property("filterExpression", _filterExpression, o.filterExpression);
  return this->ignoreAndContinue();
}

FilterJsonHandler::FilterJsonHandler() noexcept = default;

void FilterJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::Filter* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* FilterJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "description")
    return property("description", _description, o.description);
  if (str == "isDefaultFilter")
    return property("isDefaultFilter", _isDefaultFilter, o.isDefaultFilter);
  if (str == "isVisible")
    return property("isVisible", _isVisible, o.isVisible);
  if (str == "filterBlocks")
    return property("filterBlocks", _filterBlocks, o.filterBlocks);
  return this->ignoreAndContinue();
}

AttributeStatsJsonHandler::AttributeStatsJsonHandler() noexcept = default;

void AttributeStatsJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::AttributeStats* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
AttributeStatsJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "fieldName")
    return property("fieldName", _fieldName, o.fieldName);
  if (str == "label")
    return property("label", _label, o.label);
  if (str == "modelName")
    return property("modelName", _modelName, o.modelName);
  if (str == "min")
    return property("min", _min, o.min);
  if (str == "max")
    return property("max", _max, o.max);
  if (str == "subLayerIds")
    return property("subLayerIds", _subLayerIds, o.subLayerIds);
  // "mostFrequentValues" is variant — skip.
  return this->ignoreAndContinue();
}

BuildingStatsJsonHandler::BuildingStatsJsonHandler() noexcept = default;

void BuildingStatsJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::BuildingStats* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
BuildingStatsJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "summary")
    return property("summary", _summary, _pObject->summary);
  return this->ignoreAndContinue();
}

BuildingLayerJsonHandler::BuildingLayerJsonHandler() noexcept = default;

void BuildingLayerJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::BuildingLayer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
BuildingLayerJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "version")
    return property("version", _version, o.version);
  if (str == "alias")
    return property("alias", _alias, o.alias);
  if (str == "description")
    return property("description", _description, o.description);
  if (str == "copyrightText")
    return property("copyrightText", _copyrightText, o.copyrightText);
  if (str == "fullExtent")
    return property("fullExtent", _fullExtent, o.fullExtent);
  if (str == "spatialReference")
    return property("spatialReference", _spatialReference, o.spatialReference);
  if (str == "heightModelInfo")
    return property("heightModelInfo", _heightModelInfo, o.heightModelInfo);
  if (str == "sublayers")
    return property("sublayers", _sublayers, o.sublayers);
  if (str == "filters")
    return property("filters", _filters, o.filters);
  if (str == "activeFilterID")
    return property("activeFilterID", _activeFilterID, o.activeFilterID);
  if (str == "statisticsHRef")
    return property("statisticsHRef", _statisticsHRef, o.statisticsHRef);
  if (str == "capabilities")
    return property("capabilities", _capabilities, o.capabilities);
  return this->ignoreAndContinue();
}

PointCloudVertexAttributeJsonHandler::
    PointCloudVertexAttributeJsonHandler() noexcept = default;

void PointCloudVertexAttributeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudVertexAttribute* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* PointCloudVertexAttributeJsonHandler::readObjectKey(
    const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "position")
    return property("position", _position, _pObject->position);
  return this->ignoreAndContinue();
}

PointCloudGeometrySchemaJsonHandler::
    PointCloudGeometrySchemaJsonHandler() noexcept = default;

void PointCloudGeometrySchemaJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudGeometrySchema* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler* PointCloudGeometrySchemaJsonHandler::readObjectKey(
    const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "geometryType")
    return property("geometryType", _geometryType, o.geometryType);
  if (str == "topology")
    return property("topology", _topology, o.topology);
  if (str == "encoding")
    return property("encoding", _encoding, o.encoding);
  if (str == "header")
    return property("header", _header, o.header);
  if (str == "ordering")
    return property("ordering", _ordering, o.ordering);
  if (str == "vertexAttributes")
    return property("vertexAttributes", _vertexAttributes, o.vertexAttributes);
  return this->ignoreAndContinue();
}

PointCloudIndexJsonHandler::PointCloudIndexJsonHandler() noexcept = default;

void PointCloudIndexJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudIndex* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
PointCloudIndexJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "nodeVersion")
    return property("nodeVersion", _nodeVersion, o.nodeVersion);
  if (str == "nodesPerPage")
    return property("nodesPerPage", _nodesPerPage, o.nodesPerPage);
  if (str == "boundingVolumeType")
    return property(
        "boundingVolumeType",
        _boundingVolumeType,
        o.boundingVolumeType);
  if (str == "lodSelectionMetricType")
    return property(
        "lodSelectionMetricType",
        _lodSelectionMetricType,
        o.lodSelectionMetricType);
  if (str == "href")
    return property("href", _href, o.href);
  return this->ignoreAndContinue();
}

PointCloudNodeJsonHandler::PointCloudNodeJsonHandler() noexcept = default;

void PointCloudNodeJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudNode* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
PointCloudNodeJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "resourceId")
    return property("resourceId", _resourceId, o.resourceId);
  if (str == "firstChild")
    return property("firstChild", _firstChild, o.firstChild);
  if (str == "childCount")
    return property("childCount", _childCount, o.childCount);
  if (str == "vertexCount")
    return property("vertexCount", _vertexCount, o.vertexCount);
  if (str == "obb")
    return property("obb", _obb, o.obb);
  if (str == "lodThreshold")
    return property("lodThreshold", _lodThreshold, o.lodThreshold);
  return this->ignoreAndContinue();
}

PointCloudNodePageJsonHandler::PointCloudNodePageJsonHandler() noexcept =
    default;

void PointCloudNodePageJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudNodePage* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
PointCloudNodePageJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  if (str == "nodes")
    return property("nodes", _nodes, _pObject->nodes);
  return this->ignoreAndContinue();
}

PointCloudStoreJsonHandler::PointCloudStoreJsonHandler() noexcept = default;

void PointCloudStoreJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudStore* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
PointCloudStoreJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "version")
    return property("version", _version, o.version);
  if (str == "extent")
    return property("extent", _extent, o.extent);
  if (str == "index")
    return property("index", _index, o.index);
  if (str == "defaultGeometrySchema")
    return property(
        "defaultGeometrySchema",
        _defaultGeometrySchema,
        o.defaultGeometrySchema);
  if (str == "geometryEncoding")
    return property("geometryEncoding", _geometryEncoding, o.geometryEncoding);
  if (str == "attributeEncoding")
    return property(
        "attributeEncoding",
        _attributeEncoding,
        o.attributeEncoding);
  return this->ignoreAndContinue();
}

PointCloudLayerJsonHandler::PointCloudLayerJsonHandler() noexcept = default;

void PointCloudLayerJsonHandler::reset(
    IJsonHandler* pParent,
    CesiumI3S::PointCloudLayer* pObject) {
  ObjectJsonHandler::reset(pParent);
  _pObject = pObject;
}

IJsonHandler*
PointCloudLayerJsonHandler::readObjectKey(const std::string_view& str) {
  CESIUM_ASSERT(_pObject);
  auto& o = *_pObject;
  if (str == "id")
    return property("id", _id, o.id);
  if (str == "name")
    return property("name", _name, o.name);
  if (str == "alias")
    return property("alias", _alias, o.alias);
  if (str == "desc")
    return property("desc", _description, o.description);
  if (str == "copyrightText")
    return property("copyrightText", _copyrightText, o.copyrightText);
  if (str == "capabilities")
    return property("capabilities", _capabilities, o.capabilities);
  if (str == "spatialReference")
    return property("spatialReference", _spatialReference, o.spatialReference);
  if (str == "heightModelInfo")
    return property("heightModelInfo", _heightModelInfo, o.heightModelInfo);
  if (str == "serviceUpdateTimeStamp")
    return property(
        "serviceUpdateTimeStamp",
        _serviceUpdateTimeStamp,
        o.serviceUpdateTimeStamp);
  if (str == "store")
    return property("store", _store, o.store);
  if (str == "attributeStorageInfo")
    return property(
        "attributeStorageInfo",
        _attributeStorageInfo,
        o.attributeStorageInfo);
  if (str == "drawingInfo")
    return property("drawingInfo", _drawingInfo, o.drawingInfo);
  if (str == "elevationInfo")
    return property("elevationInfo", _elevationInfo, o.elevationInfo);
  if (str == "fields")
    return property("fields", _fields, o.fields);
  return this->ignoreAndContinue();
}

} // namespace CesiumI3SReader
