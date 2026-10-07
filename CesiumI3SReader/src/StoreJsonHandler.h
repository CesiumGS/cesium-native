#pragma once

#include "DefaultGeometrySchemaJsonHandlers.h"
#include "MaterialJsonHandlers.h"
#include "SpatialReferenceJsonHandlers.h"
#include "TextureDefinitionJsonHandlers.h"

#include <CesiumI3S/Store.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class StoreJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Store;
  StoreJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Store* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Store* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _id;
  EnumJsonHandler<CesiumI3S::Store::Profile> _profile;
  EnumArrayJsonHandler<CesiumI3S::Store::ResourcePattern> _resourcePattern;
  CesiumJsonReader::StringJsonHandler _rootNode;
  CesiumJsonReader::StringJsonHandler _version;
  ExtentJsonHandler _extent;
  CesiumJsonReader::StringJsonHandler _indexCrs;
  CesiumJsonReader::StringJsonHandler _vertexCrs;
  EnumJsonHandler<CesiumI3S::Store::NormalReferenceFrame> _normalReferenceFrame;
  CesiumJsonReader::StringJsonHandler _nidEncoding;
  CesiumJsonReader::StringJsonHandler _featureEncoding;
  CesiumJsonReader::StringJsonHandler _geometryEncoding;
  CesiumJsonReader::StringJsonHandler _attributeEncoding;
  CesiumJsonReader::
      ArrayJsonHandler<std::string, CesiumJsonReader::StringJsonHandler>
          _textureEncoding;
  EnumJsonHandler<CesiumI3S::Store::LodType> _lodType;
  EnumJsonHandler<CesiumI3S::Store::LodModel> _lodModel;
  CesiumJsonReader::StringJsonHandler _indexingScheme;
  GeometrySchemaJsonHandler _defaultGeometrySchema;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::TextureDefinitionInfo,
      TextureDefinitionInfoJsonHandler>
      _defaultTextureDefinition;
  CesiumJsonReader::DictionaryJsonHandler<
      CesiumI3S::MaterialDefinitionInfo,
      MaterialDefinitionInfoJsonHandler>
      _defaultMaterialDefinition;
};

} // namespace CesiumI3SReader
