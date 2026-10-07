#pragma once

#include "EnumJsonHandler.h"
#include "FixedArrayJsonHandler.h"

#include <CesiumI3S/Material.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/BoolJsonHandler.h>
#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class TextureSlotReferenceJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::TextureSlotReference;
  TextureSlotReferenceJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::TextureSlotReference* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::TextureSlotReference* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _textureSetDefinitionId;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _texCoord;
  CesiumJsonReader::DoubleJsonHandler _factor;
};

class PbrMetallicRoughnessJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::PbrMetallicRoughness;
  PbrMetallicRoughnessJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::PbrMetallicRoughness* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::PbrMetallicRoughness* _pObject = nullptr;
  FixedArrayJsonHandler<double, 4> _baseColorFactor;
  TextureSlotReferenceJsonHandler _baseColorTexture;
  CesiumJsonReader::DoubleJsonHandler _metallicFactor;
  CesiumJsonReader::DoubleJsonHandler _roughnessFactor;
  TextureSlotReferenceJsonHandler _metallicRoughnessTexture;
};

class MaterialDefinitionJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MaterialDefinition;
  MaterialDefinitionJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MaterialDefinition* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MaterialDefinition* _pObject = nullptr;
  PbrMetallicRoughnessJsonHandler _pbrMetallicRoughness;
  TextureSlotReferenceJsonHandler _normalTexture;
  TextureSlotReferenceJsonHandler _occlusionTexture;
  TextureSlotReferenceJsonHandler _emissiveTexture;
  FixedArrayJsonHandler<double, 3> _emissiveFactor;
  EnumJsonHandler<CesiumI3S::MaterialDefinition::AlphaMode> _alphaMode;
  CesiumJsonReader::DoubleJsonHandler _alphaCutoff;
  CesiumJsonReader::BoolJsonHandler _doubleSided;
  EnumJsonHandler<CesiumI3S::MaterialDefinition::CullFace> _cullFace;
};

class MaterialParamsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MaterialParams;
  MaterialParamsJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MaterialParams* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MaterialParams* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _transparency;
  CesiumJsonReader::DoubleJsonHandler _reflectivity;
  CesiumJsonReader::DoubleJsonHandler _shininess;
  CesiumJsonReader::
      ArrayJsonHandler<double, CesiumJsonReader::DoubleJsonHandler>
          _ambient;
  CesiumJsonReader::
      ArrayJsonHandler<double, CesiumJsonReader::DoubleJsonHandler>
          _diffuse;
  CesiumJsonReader::
      ArrayJsonHandler<double, CesiumJsonReader::DoubleJsonHandler>
          _specular;
  EnumJsonHandler<CesiumI3S::MaterialParams::RenderMode> _renderMode;
  CesiumJsonReader::BoolJsonHandler _castShadows;
  CesiumJsonReader::BoolJsonHandler _receiveShadows;
  EnumJsonHandler<CesiumI3S::MaterialDefinition::CullFace> _cullFace;
  CesiumJsonReader::BoolJsonHandler _vertexColors;
  CesiumJsonReader::BoolJsonHandler _vertexRegions;
  CesiumJsonReader::BoolJsonHandler _useVertexColorAlpha;
};

class MaterialDefinitionInfoJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MaterialDefinitionInfo;
  MaterialDefinitionInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MaterialDefinitionInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MaterialDefinitionInfo* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _name;
  EnumJsonHandler<CesiumI3S::MaterialDefinitionInfo::Type> _type;
  CesiumJsonReader::StringJsonHandler _sharedResourceHref;
  MaterialParamsJsonHandler _params;
};

} // namespace CesiumI3SReader
