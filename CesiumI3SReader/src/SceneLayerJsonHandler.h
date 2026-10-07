#pragma once

#include "AttributeStorageJsonHandlers.h"
#include "DrawingInfoJsonHandlers.h"
#include "FieldsJsonHandlers.h"
#include "FixedArrayJsonHandler.h"
#include "GeometryJsonHandlers.h"
#include "MaterialJsonHandlers.h"
#include "NodeJsonHandlers.h"
#include "SpatialReferenceJsonHandlers.h"
#include "StatsJsonHandlers.h"
#include "StoreJsonHandler.h"
#include "TextureDefinitionJsonHandlers.h"

#include <CesiumI3S/Scene.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/BoolJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class TimeInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::TimeInfo;
  TimeInfoJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::TimeInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::TimeInfo* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _endTimeField;
  CesiumJsonReader::StringJsonHandler _startTimeField;
};

class RangeInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::RangeInfo;
  RangeInfoJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::RangeInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::RangeInfo* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _field;
  CesiumJsonReader::StringJsonHandler _name;
};

class LayerJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Layer;
  LayerJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Layer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Layer* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _id;
  CesiumJsonReader::StringJsonHandler _href;
  EnumJsonHandler<CesiumI3S::SceneLayerType> _layerType;
  SpatialReferenceJsonHandler _spatialReference;
  HeightModelInfoJsonHandler _heightModelInfo;
  CesiumJsonReader::StringJsonHandler _version;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _alias;
  CesiumJsonReader::StringJsonHandler _description;
  CesiumJsonReader::StringJsonHandler _copyrightText;
  EnumArrayJsonHandler<CesiumI3S::LayerCapabilities> _capabilities;
  CachedDrawingInfoJsonHandler _cachedDrawingInfo;
  DrawingInfoJsonHandler _drawingInfo;
  ElevationInfoJsonHandler _elevationInfo;
  ServiceUpdateTimeStampJsonHandler _serviceUpdateTimeStamp;
  StoreJsonHandler _store;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Field, FieldJsonHandler>
      _fields;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::AttributeStorageInfo,
      AttributeStorageInfoJsonHandler>
      _attributeStorageInfo;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::StatisticsInfo, StatisticsInfoJsonHandler>
          _statisticsInfo;
  NodePageDefinitionJsonHandler _nodePages;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::GeometryDefinition,
      GeometryDefinitionJsonHandler>
      _geometryDefinitions;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::MaterialDefinition,
      MaterialDefinitionJsonHandler>
      _materialDefinitions;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::TextureSetDefinition,
      TextureSetDefinitionJsonHandler>
      _textureSetDefinitions;
  FullExtentJsonHandler _fullExtent;
  CesiumJsonReader::DoubleJsonHandler _zFactor;
  // popupInfo is raw JSON — skipped
  CesiumJsonReader::BoolJsonHandler _disablePopup;
  TimeInfoJsonHandler _timeInfo;
  RangeInfoJsonHandler _rangeInfo;
};

} // namespace CesiumI3SReader
