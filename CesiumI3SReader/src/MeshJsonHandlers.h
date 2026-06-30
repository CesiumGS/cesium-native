#pragma once

#include <CesiumI3S/Mesh.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>

namespace CesiumI3SReader {

class MeshMaterialJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MeshMaterial;
  MeshMaterialJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MeshMaterial* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MeshMaterial* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _definition;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _resource;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _texelCountHint;
};

class MeshGeometryJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MeshGeometry;
  MeshGeometryJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MeshGeometry* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MeshGeometry* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _definition;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _resource;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _vertexCount;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _featureCount;
};

class MeshAttributeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MeshAttribute;
  MeshAttributeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MeshAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MeshAttribute* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _resource;
};

class MeshJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Mesh;
  MeshJsonHandler() noexcept;
  void reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Mesh* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Mesh* _pObject = nullptr;
  MeshGeometryJsonHandler _geometry;
  MeshMaterialJsonHandler _material;
  MeshAttributeJsonHandler _attribute;
};

} // namespace CesiumI3SReader
