#pragma once

#include "AttributeStorageJsonHandlers.h"
#include "EnumJsonHandler.h"

#include <CesiumI3S/GeometrySchema.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class GeometryAttributeJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::GeometryAttribute;
  GeometryAttributeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::GeometryAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::GeometryAttribute* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _byteOffset;
  EnumJsonHandler<CesiumI3S::DataType> _valueType;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _valuesPerElement;
};

class VertexAttributeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::VertexAttribute;
  VertexAttributeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::VertexAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::VertexAttribute* _pObject = nullptr;
  GeometryAttributeJsonHandler _position;
  GeometryAttributeJsonHandler _normal;
  GeometryAttributeJsonHandler _uv0;
  GeometryAttributeJsonHandler _color;
  GeometryAttributeJsonHandler _region;
};

class GeometrySchemaJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::GeometrySchema;
  GeometrySchemaJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::GeometrySchema* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::GeometrySchema* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::GeometryTopology> _geometryType;
  EnumJsonHandler<CesiumI3S::GeometrySchema::BufferLayout> _topology;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::HeaderAttribute, HeaderAttributeJsonHandler>
          _header;
  EnumArrayJsonHandler<CesiumI3S::GeometrySchema::AttributeField> _ordering;
  VertexAttributeJsonHandler _vertexAttributes;
  VertexAttributeJsonHandler _faces;
  CesiumJsonReader::
      ArrayJsonHandler<std::string, CesiumJsonReader::StringJsonHandler>
          _featureAttributeOrder;
  FeatureAttributeJsonHandler _featureAttributes;
};

} // namespace CesiumI3SReader
