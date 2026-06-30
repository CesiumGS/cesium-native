#pragma once

#include "MeshJsonHandlers.h"
#include "SpatialReferenceJsonHandlers.h"

#include <CesiumI3S/Node.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class NodePageDefinitionJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::NodePageDefinition;
  NodePageDefinitionJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::NodePageDefinition* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::NodePageDefinition* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _nodesPerPage;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _rootIndex;
  EnumJsonHandler<CesiumI3S::LodSelection::Metric> _lodSelectionMetricType;
};

class NodeReferenceJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::NodeReference;
  NodeReferenceJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::NodeReference* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::NodeReference* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _id;
  MinimumBoundingSphereJsonHandler _mbs;
  CesiumJsonReader::StringJsonHandler _href;
  CesiumJsonReader::StringJsonHandler _version;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _featureCount;
  ObbJsonHandler _obb;
};

class NodeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Node;
  NodeJsonHandler() noexcept;
  void reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Node* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Node* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _index;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _parentIndex;
  CesiumJsonReader::DoubleJsonHandler _lodThreshold;
  ObbJsonHandler _obb;
  CesiumJsonReader::
      ArrayJsonHandler<uint32_t, CesiumJsonReader::IntegerJsonHandler<uint32_t>>
          _children;
  MeshJsonHandler _mesh;
};

class NodePageJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::NodePage;
  NodePageJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::NodePage* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::NodePage* _pObject = nullptr;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Node, NodeJsonHandler> _nodes;
};

} // namespace CesiumI3SReader
