#pragma once

#include "EnumJsonHandler.h"
#include "FixedArrayJsonHandler.h"

#include <CesiumI3S/Geometry.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>

namespace CesiumI3SReader {

class CompressedAttributesJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Compression;
  CompressedAttributesJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::Compression* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Compression* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::Compression::Encoding> _encoding;
  EnumArrayJsonHandler<CesiumI3S::Compression::Attribute> _attributes;
};

/** @brief Reads a geometry attribute descriptor and populates
 * `component` and `binding` into a `GeometryBuffer::Attribute`. */
class GeometryAttributeDescJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::GeometryBuffer::Attribute;
  GeometryAttributeDescJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::GeometryBuffer::Attribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::GeometryBuffer::Attribute* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _component;
  EnumJsonHandler<CesiumI3S::GeometryBuffer::Attribute::Binding> _binding;
};

/** @brief Reads a feature ID attribute descriptor and populates `component`,
 * `binding`, and `type` into a `GeometryBuffer::FeatureIdAttribute`. */
class FeatureIdAttributeDescJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::GeometryBuffer::FeatureIdAttribute;
  FeatureIdAttributeDescJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::GeometryBuffer::FeatureIdAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::GeometryBuffer::FeatureIdAttribute* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _component;
  EnumJsonHandler<CesiumI3S::GeometryBuffer::Attribute::Binding> _binding;
  EnumJsonHandler<CesiumI3S::GeometryBuffer::FeatureIdAttribute::Type> _type;
};

class GeometryBufferJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::GeometryBuffer;
  GeometryBufferJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::GeometryBuffer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::GeometryBuffer* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _offset;
  GeometryAttributeDescJsonHandler _position;
  GeometryAttributeDescJsonHandler _normal;
  GeometryAttributeDescJsonHandler _uv0;
  GeometryAttributeDescJsonHandler _color;
  GeometryAttributeDescJsonHandler _uvRegion;
  FeatureIdAttributeDescJsonHandler _featureId;
  GeometryAttributeDescJsonHandler _faceRange;
  CompressedAttributesJsonHandler _compressedAttributes;
};

class GeometryDefinitionJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::GeometryDefinition;
  GeometryDefinitionJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::GeometryDefinition* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::GeometryDefinition* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::GeometryTopology> _topology;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::GeometryBuffer, GeometryBufferJsonHandler>
          _geometryBuffers;
};

class GeometryJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Geometry;
  GeometryJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Geometry* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Geometry* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _id;
  EnumJsonHandler<CesiumI3S::Geometry::Storage> _type;
  FixedArrayJsonHandler<double, 16> _transformation;
  // params is raw JSON — skipped
};

} // namespace CesiumI3SReader
