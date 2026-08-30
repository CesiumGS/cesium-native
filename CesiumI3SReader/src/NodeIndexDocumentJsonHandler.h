#pragma once

#include "FeatureDataJsonHandler.h"
#include "FixedArrayJsonHandler.h"
#include "LodSelectionJsonHandler.h"
#include "NodeJsonHandlers.h"
#include "ResourceJsonHandler.h"

#include <CesiumI3S/NodeIndexDocument.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class NodeIndexDocumentJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::NodeIndexDocument;
  NodeIndexDocumentJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::NodeIndexDocument* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::NodeIndexDocument* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _id;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _level;
  CesiumJsonReader::StringJsonHandler _version;
  MinimumBoundingSphereJsonHandler _mbs;
  ObbJsonHandler _obb;
  FixedArrayJsonHandler<double, 16> _transform;
  NodeReferenceJsonHandler _parentNode;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::NodeReference, NodeReferenceJsonHandler>
          _children;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::NodeReference, NodeReferenceJsonHandler>
          _neighbors;
  ResourceJsonHandler _sharedResource;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Resource, ResourceJsonHandler>
      _featureData;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Resource, ResourceJsonHandler>
      _geometryData;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Resource, ResourceJsonHandler>
      _textureData;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Resource, ResourceJsonHandler>
      _attributeData;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::LodSelection, LodSelectionJsonHandler>
          _lodSelection;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::FeatureData, FeatureDataJsonHandler>
          _features;
};

} // namespace CesiumI3SReader
