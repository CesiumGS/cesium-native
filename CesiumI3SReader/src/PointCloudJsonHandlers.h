#pragma once

#include "AttributeStorageJsonHandlers.h"
#include "DrawingInfoJsonHandlers.h"
#include "EnumJsonHandler.h"
#include "FieldsJsonHandlers.h"
#include "FixedArrayJsonHandler.h"
#include "SpatialReferenceJsonHandlers.h"

#include <CesiumI3S/PointCloud.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class PointCloudVertexAttributeJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudVertexAttribute;
  PointCloudVertexAttributeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudVertexAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudVertexAttribute* _pObject = nullptr;
  ValueJsonHandler _position;
};

class PointCloudGeometrySchemaJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudGeometrySchema;
  PointCloudGeometrySchemaJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudGeometrySchema* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudGeometrySchema* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::GeometryTopology> _geometryType;
  EnumJsonHandler<CesiumI3S::GeometrySchema::BufferLayout> _topology;
  EnumJsonHandler<CesiumI3S::Compression::Encoding> _encoding;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::HeaderAttribute, HeaderAttributeJsonHandler>
          _header;
  CesiumJsonReader::
      ArrayJsonHandler<std::string, CesiumJsonReader::StringJsonHandler>
          _ordering;
  PointCloudVertexAttributeJsonHandler _vertexAttributes;
};

class PointCloudIndexJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudIndex;
  PointCloudIndexJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudIndex* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudIndex* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _nodeVersion;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _nodesPerPage;
  EnumJsonHandler<CesiumI3S::BoundingVolume> _boundingVolumeType;
  EnumJsonHandler<CesiumI3S::LodSelection::Metric> _lodSelectionMetricType;
  CesiumJsonReader::StringJsonHandler _href;
};

class PointCloudNodeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudNode;
  PointCloudNodeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudNode* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudNode* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _resourceId;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _firstChild;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _childCount;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _vertexCount;
  ObbJsonHandler _obb;
  CesiumJsonReader::DoubleJsonHandler _lodThreshold;
};

class PointCloudNodePageJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudNodePage;
  PointCloudNodePageJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudNodePage* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudNodePage* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::PointCloudNode, PointCloudNodeJsonHandler>
          _nodes;
};

class PointCloudStoreJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudStore;
  PointCloudStoreJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudStore* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudStore* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _id;
  CesiumJsonReader::StringJsonHandler _version;
  ExtentJsonHandler _extent;
  PointCloudIndexJsonHandler _index;
  PointCloudGeometrySchemaJsonHandler _defaultGeometrySchema;
  CesiumJsonReader::StringJsonHandler _geometryEncoding;
  CesiumJsonReader::StringJsonHandler _attributeEncoding;
};

class PointCloudLayerJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PointCloudLayer;
  PointCloudLayerJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PointCloudLayer* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PointCloudLayer* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _id;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _alias;
  CesiumJsonReader::StringJsonHandler _description;
  CesiumJsonReader::StringJsonHandler _copyrightText;
  EnumArrayJsonHandler<CesiumI3S::LayerCapabilities> _capabilities;
  SpatialReferenceJsonHandler _spatialReference;
  HeightModelInfoJsonHandler _heightModelInfo;
  ServiceUpdateTimeStampJsonHandler _serviceUpdateTimeStamp;
  PointCloudStoreJsonHandler _store;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::AttributeStorageInfo,
      AttributeStorageInfoJsonHandler>
      _attributeStorageInfo;
  DrawingInfoJsonHandler _drawingInfo;
  ElevationInfoJsonHandler _elevationInfo;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Field, FieldJsonHandler>
      _fields;
};

} // namespace CesiumI3SReader
