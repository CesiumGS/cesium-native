#pragma once
#include <CesiumI3S/Layer.h>
#include <CesiumI3S/Library.h>
#include <CesiumI3S/SpatialReference.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace CesiumI3S {

/** @brief Recursive sublayer/group node within a building scene layer
 * (sublayer.bld.md). */
struct CESIUMI3S_API Sublayer {
  /** @brief Discipline classification (sublayer.bld.md). */
  enum class Discipline {
    Mechanical,
    Architectural,
    Piping,
    Electrical,
    Structural,
    Infrastructure
  };
  /** @brief Type of a sublayer (sublayer.bld.md). */
  enum class Type { Group, ThreeDObject, Point };
  /** @brief Unique identifier of the sublayer. */
  uint32_t id = 0;
  /** @brief Display name of the sublayer. */
  std::string name;
  /** @brief Short alias for display. */
  std::optional<std::string> alias;
  /** @brief Discipline category. */
  std::optional<Discipline> discipline;
  /** @brief Well-known model name used for symbology and filtering. */
  std::optional<std::string> modelName;
  /** @brief Sublayer type. */
  Type layerType = Type::ThreeDObject;
  /** @brief Default visibility. */
  bool visibility = true;
  /** @brief Child sublayers (recursive). */
  std::optional<std::vector<Sublayer>> sublayers;
  /** @brief When true, the sublayer contains no features. */
  std::optional<bool> isEmpty;
};

/** @brief Filter display mode (filterMode.bld.md). */
struct CESIUMI3S_API FilterMode {
  /** @brief Display mode variant (filterMode.bld.md). */
  enum class Type { Solid, WireFrame };
  /** @brief Display mode type. */
  Type type = Type::Solid;
  /** @brief Edge styling; raw JSON per ArcGIS Web Scene Specification
   * (WireFrame only). */
  std::optional<std::string> edges;
};

/** @brief Defines which elements are drawn and how (filterBlock.bld.md). */
struct CESIUMI3S_API FilterBlock {
  /** @brief Display title of the filter block. */
  std::string title;
  /** @brief Display mode applied to matching elements. */
  FilterMode filterMode;
  /** @brief SQL-like expression selecting features. */
  std::string filterExpression;
};

/** @brief Named filter for a building scene layer (filter.bld.md). */
struct CESIUMI3S_API Filter {
  /** @brief Unique filter identifier. */
  std::string id;
  /** @brief Display name of the filter. */
  std::string name;
  /** @brief Description of the filter. */
  std::string description;
  /** @brief When true, this filter is active by default. */
  std::optional<bool> isDefaultFilter;
  /** @brief When true, this filter is shown in the UI. */
  std::optional<bool> isVisible;
  /** @brief Filter blocks that make up this filter. */
  std::vector<FilterBlock> filterBlocks;
};

/** @brief Per-attribute statistics aggregated across sublayers
 * (attributestats.bld.md). */
struct CESIUMI3S_API AttributeStats {
  /** @brief Well-known model name for this attribute (attributestats.bld.md).
   */
  enum class ModelName {
    Category,
    Family,
    FamilyType,
    BldgLevel,
    CreatedPhase,
    DemolishedPhase,
    Discipline,
    AssemblyCode,
    OmniClass,
    SystemClassifications,
    SystemType,
    SystemName,
    SystemClass,
    Custom
  };
  /** @brief Attribute field name. */
  std::string fieldName;
  /** @brief Display label. */
  std::optional<std::string> label;
  /** @brief Well-known model name for this attribute. */
  std::optional<ModelName> modelName;
  /** @brief Minimum value. */
  std::optional<double> min;
  /** @brief Maximum value. */
  std::optional<double> max;
  /** @brief Most frequent values; either all integers or all strings. */
  std::optional<std::variant<std::vector<int64_t>, std::vector<std::string>>>
      mostFrequentValues;
  /** @brief Sublayer IDs contributing to these stats. */
  std::vector<uint32_t> subLayerIds;
};

/** @brief Summary statistics for all sublayers of a building scene layer
 * (stats.bld.md). */
struct CESIUMI3S_API BuildingStats {
  /** @brief Per-attribute statistics entries. */
  std::vector<AttributeStats> summary;
};

/** @brief Building Scene Layer definition; layerType is always "Building"
 * (layer.bld.md). */
struct CESIUMI3S_API BuildingLayer {
  /** @brief Unique layer ID within the service. */
  uint32_t id = 0;
  /** @brief Layer name. */
  std::string name;
  /** @brief Version of the layer (store update session ID). */
  std::string version;
  /** @brief Alias name for display. */
  std::optional<std::string> alias;
  /** @brief Layer description. */
  std::optional<std::string> description;
  /** @brief Copyright and usage information. */
  std::optional<std::string> copyrightText;
  /** @brief 3D geographic extent of the layer. */
  FullExtent fullExtent;
  /** @brief Spatial reference of the layer. */
  SpatialReference spatialReference;
  /** @brief Height model metadata. */
  std::optional<HeightModelInfo> heightModelInfo;
  /** @brief Sublayer tree. */
  std::vector<Sublayer> sublayers;
  /** @brief Available filters for the layer. */
  std::optional<std::vector<Filter>> filters;
  /** @brief ID of the currently active filter. */
  std::optional<std::string> activeFilterID;
  /** @brief Relative URL to the statistics resource. */
  std::optional<std::string> statisticsHRef;
  /** @brief Operations supported by the layer. */
  std::vector<LayerCapabilities> capabilities;
};

} // namespace CesiumI3S
