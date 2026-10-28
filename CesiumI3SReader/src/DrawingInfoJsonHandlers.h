#pragma once

#include "EnumJsonHandler.h"

#include <CesiumI3S/DrawingInfo.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/BoolJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class RendererEdgesJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::RendererEdges;
  RendererEdgesJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::RendererEdges* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::RendererEdges* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<uint8_t, CesiumJsonReader::IntegerJsonHandler<uint8_t>>
          _color;
  CesiumJsonReader::DoubleJsonHandler _transparency;
};

class RendererMaterialJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::RendererMaterial;
  RendererMaterialJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::RendererMaterial* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::RendererMaterial* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _colorMixMode;
  CesiumJsonReader::
      ArrayJsonHandler<uint8_t, CesiumJsonReader::IntegerJsonHandler<uint8_t>>
          _color;
  CesiumJsonReader::DoubleJsonHandler _transparency;
};

class SymbolLayerJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::SymbolLayer;
  SymbolLayerJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::SymbolLayer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::SymbolLayer* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _type;
  RendererEdgesJsonHandler _edges;
  RendererEdgesJsonHandler _outline;
  RendererMaterialJsonHandler _material;
};

class SymbolJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Symbol;
  SymbolJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Symbol* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Symbol* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::SymbolLayer, SymbolLayerJsonHandler>
          _symbolLayers;
};

class UniqueValueInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::UniqueValueInfo;
  UniqueValueInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::UniqueValueInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::UniqueValueInfo* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _value;
  SymbolJsonHandler _symbol;
};

class UniqueValueClassJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::UniqueValueClass;
  UniqueValueClassJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::UniqueValueClass* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::UniqueValueClass* _pObject = nullptr;
  SymbolJsonHandler _symbol;
  CesiumJsonReader::ArrayJsonHandler<
      std::vector<std::string>,
      CesiumJsonReader::
          ArrayJsonHandler<std::string, CesiumJsonReader::StringJsonHandler>>
      _values;
};

class UniqueValueGroupJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::UniqueValueGroup;
  UniqueValueGroupJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::UniqueValueGroup* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::UniqueValueGroup* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::UniqueValueClass, UniqueValueClassJsonHandler>
          _classes;
};

class ClassBreakInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::ClassBreakInfo;
  ClassBreakInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::ClassBreakInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::ClassBreakInfo* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _classMinValue;
  CesiumJsonReader::DoubleJsonHandler _classMaxValue;
  SymbolJsonHandler _symbol;
};

class RendererJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Renderer;
  RendererJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Renderer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Renderer* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::Renderer::Type> _type;
  SymbolJsonHandler _symbol;
  SymbolJsonHandler _defaultSymbol;
  CesiumJsonReader::StringJsonHandler _field1;
  CesiumJsonReader::StringJsonHandler _field2;
  CesiumJsonReader::StringJsonHandler _field3;
  CesiumJsonReader::StringJsonHandler _field;
  CesiumJsonReader::DoubleJsonHandler _minValue;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::UniqueValueInfo, UniqueValueInfoJsonHandler>
          _uniqueValueInfos;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::UniqueValueGroup, UniqueValueGroupJsonHandler>
          _uniqueValueGroups;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::ClassBreakInfo, ClassBreakInfoJsonHandler>
          _classBreakInfos;
};

class CachedDrawingInfoJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::CachedDrawingInfo;
  CachedDrawingInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::CachedDrawingInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::CachedDrawingInfo* _pObject = nullptr;
  CesiumJsonReader::BoolJsonHandler _color;
};

class DrawingInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::DrawingInfo;
  DrawingInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::DrawingInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::DrawingInfo* _pObject = nullptr;
  RendererJsonHandler _renderer;
  CesiumJsonReader::BoolJsonHandler _scaleSymbols;
};

class ServiceUpdateTimeStampJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::ServiceUpdateTimeStamp;
  ServiceUpdateTimeStampJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::ServiceUpdateTimeStamp* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::ServiceUpdateTimeStamp* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _lastUpdate;
};

} // namespace CesiumI3SReader
