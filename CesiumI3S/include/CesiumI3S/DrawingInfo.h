#pragma once
#include <CesiumI3S/Library.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Edge or outline stroke descriptor within a Fill symbol layer
 * (ArcGIS Web Scene Specification). */
struct CESIUMI3S_API RendererEdges {
  /** @brief RGB or RGBA color as byte values 0-255. */
  std::optional<std::vector<uint8_t>> color;
  /** @brief Transparency percentage: 0 = fully opaque, 100 = fully
   * transparent. */
  std::optional<double> transparency;
};

/** @brief Surface material descriptor within a Fill symbol layer. */
struct CESIUMI3S_API RendererMaterial {
  /** @brief Color mixing mode, e.g. "replace", "multiply", or "tint". */
  std::optional<std::string> colorMixMode;
  /** @brief RGB or RGBA color as byte values 0-255. */
  std::optional<std::vector<uint8_t>> color;
  /** @brief Transparency percentage: 0 = fully opaque, 100 = fully
   * transparent. */
  std::optional<double> transparency;
};

/** @brief A single layer within a symbol definition.
 * Only "Fill" layers are modelled; unknown types are retained via @ref type
 * but their sub-fields are not parsed. */
struct CESIUMI3S_API SymbolLayer {
  /** @brief Layer type identifier, e.g. "Fill". */
  std::string type;
  /** @brief Edge strokes; populated when type == "Fill". */
  std::optional<RendererEdges> edges;
  /** @brief Outline strokes; populated when type == "Fill". */
  std::optional<RendererEdges> outline;
  /** @brief Surface material; populated when type == "Fill". */
  std::optional<RendererMaterial> material;
};

/** @brief Symbol definition referenced from renderer entries. */
struct CESIUMI3S_API Symbol {
  std::vector<SymbolLayer> symbolLayers;
};

/** @brief One entry in a uniqueValue renderer's uniqueValueInfos array. */
struct CESIUMI3S_API UniqueValueInfo {
  /** @brief Attribute value (stringified) that selects this symbol. */
  std::string value;
  Symbol symbol;
};

/** @brief One class within a uniqueValueGroups entry. */
struct CESIUMI3S_API UniqueValueClass {
  Symbol symbol;
  /** @brief Outer dimension: per-value tuple; inner: one string per field. */
  std::vector<std::vector<std::string>> values;
};

/** @brief One group in a uniqueValue renderer's uniqueValueGroups array. */
struct CESIUMI3S_API UniqueValueGroup {
  std::vector<UniqueValueClass> classes;
};

/** @brief One range entry in a classBreaks renderer. */
struct CESIUMI3S_API ClassBreakInfo {
  std::optional<double> classMinValue;
  std::optional<double> classMaxValue;
  Symbol symbol;
};

/** @brief Renderer definition from the ArcGIS Web Scene Specification.
 * The set of populated fields depends on @ref type:
 * - Simple: @ref symbol
 * - UniqueValue: @ref defaultSymbol, @ref field1, @ref field2, @ref field3,
 *   @ref uniqueValueInfos, @ref uniqueValueGroups
 * - ClassBreaks: @ref defaultSymbol, @ref field, @ref minValue,
 *   @ref classBreakInfos */
struct CESIUMI3S_API Renderer {
  /** @brief Renderer variant. */
  enum class Type { Simple, UniqueValue, ClassBreaks };

  Type type = Type::Simple;

  // simple
  std::optional<Symbol> symbol;

  // uniqueValue and classBreaks
  std::optional<Symbol> defaultSymbol;

  // uniqueValue
  std::optional<std::string> field1;
  std::optional<std::string> field2;
  std::optional<std::string> field3;
  std::vector<UniqueValueInfo> uniqueValueInfos;
  std::vector<UniqueValueGroup> uniqueValueGroups;

  // classBreaks
  std::optional<std::string> field;
  std::optional<double> minValue;
  std::vector<ClassBreakInfo> classBreakInfos;
};

/** @brief Indicates whether drawingInfo colour is captured in the binary
 * representation (cachedDrawingInfo.cmn.md). */
struct CESIUMI3S_API CachedDrawingInfo {
  /** @brief True if the color is cached in the binary representation. */
  bool color = false;
};

/** @brief Drawing and symbology information for a layer
 * (drawingInfo.cmn.md). */
struct CESIUMI3S_API DrawingInfo {
  Renderer renderer;
  /** @brief True if symbols are scaled with distance. */
  std::optional<bool> scaleSymbols;
};

/** @brief Timestamp for the last service or source data update
 * (serviceUpdateTimeStamp.cmn.md). */
struct CESIUMI3S_API ServiceUpdateTimeStamp {
  /** @brief Unix timestamp (milliseconds) of the last update. */
  uint64_t lastUpdate = 0;
};

} // namespace CesiumI3S
