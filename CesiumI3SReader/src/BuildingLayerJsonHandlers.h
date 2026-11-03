#pragma once

#include "EnumJsonHandler.h"
#include "FixedArrayJsonHandler.h"
#include "SceneLayerJsonHandler.h"
#include "SpatialReferenceJsonHandlers.h"
#include "StatsJsonHandlers.h"

#include <CesiumI3S/Building.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/BoolJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

// Forward declaration for recursive SublayerJsonHandler.
class SublayerJsonHandler;

class SublayerJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Sublayer;
  SublayerJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Sublayer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Sublayer* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _id;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _alias;
  EnumJsonHandler<CesiumI3S::Sublayer::Discipline> _discipline;
  CesiumJsonReader::StringJsonHandler _modelName;
  EnumJsonHandler<CesiumI3S::Sublayer::Type> _layerType;
  CesiumJsonReader::BoolJsonHandler _visibility;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Sublayer, SublayerJsonHandler>
      _sublayers;
  CesiumJsonReader::BoolJsonHandler _isEmpty;
};

class FilterModeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::FilterMode;
  FilterModeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::FilterMode* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::FilterMode* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::FilterMode::Type> _type;
  // edges is raw JSON — skipped
};

class FilterBlockJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::FilterBlock;
  FilterBlockJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::FilterBlock* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::FilterBlock* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _title;
  FilterModeJsonHandler _filterMode;
  CesiumJsonReader::StringJsonHandler _filterExpression;
};

class FilterJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Filter;
  FilterJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Filter* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Filter* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _id;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _description;
  CesiumJsonReader::BoolJsonHandler _isDefaultFilter;
  CesiumJsonReader::BoolJsonHandler _isVisible;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::FilterBlock, FilterBlockJsonHandler>
          _filterBlocks;
};

class AttributeStatsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::AttributeStats;
  AttributeStatsJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::AttributeStats* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::AttributeStats* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _fieldName;
  CesiumJsonReader::StringJsonHandler _label;
  EnumJsonHandler<CesiumI3S::AttributeStats::ModelName> _modelName;
  CesiumJsonReader::DoubleJsonHandler _min;
  CesiumJsonReader::DoubleJsonHandler _max;
  // mostFrequentValues is variant<vector<int64_t>, vector<string>> — skipped
  CesiumJsonReader::
      ArrayJsonHandler<uint32_t, CesiumJsonReader::IntegerJsonHandler<uint32_t>>
          _subLayerIds;
};

class BuildingStatsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::BuildingStats;
  BuildingStatsJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::BuildingStats* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::BuildingStats* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::AttributeStats, AttributeStatsJsonHandler>
          _summary;
};

class BuildingLayerJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::BuildingLayer;
  BuildingLayerJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::BuildingLayer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::BuildingLayer* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _id;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _version;
  CesiumJsonReader::StringJsonHandler _alias;
  CesiumJsonReader::StringJsonHandler _description;
  CesiumJsonReader::StringJsonHandler _copyrightText;
  FullExtentJsonHandler _fullExtent;
  SpatialReferenceJsonHandler _spatialReference;
  HeightModelInfoJsonHandler _heightModelInfo;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Sublayer, SublayerJsonHandler>
      _sublayers;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Filter, FilterJsonHandler>
      _filters;
  CesiumJsonReader::StringJsonHandler _activeFilterID;
  CesiumJsonReader::StringJsonHandler _statisticsHRef;
  EnumArrayJsonHandler<CesiumI3S::LayerCapabilities> _capabilities;
};

} // namespace CesiumI3SReader
