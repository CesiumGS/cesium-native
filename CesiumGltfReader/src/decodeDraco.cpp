#include "decodeDraco.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Tracing.h>

#include <draco/attributes/geometry_indices.h>
#include <draco/attributes/point_attribute.h>
#include <draco/core/status_or.h>
#include <draco/mesh/mesh.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4018 4804)
#endif

#include <draco/compression/decode.h>
#include <draco/core/decoder_buffer.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace CesiumGltfReader {

namespace {
std::unique_ptr<draco::Mesh> decodeBufferViewToDracoMesh(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& /* primitive */,
    const CesiumGltf::ExtensionKhrDracoMeshCompression& draco) {
  CESIUM_TRACE("CesiumGltfReader::decodeBufferViewToDracoMesh");
  CESIUM_ASSERT(readGltf.model.has_value());
  CesiumGltf::Model& model = readGltf.model.value();

  CesiumGltf::BufferView* pBufferView =
      CesiumGltf::Model::getSafe(&model.bufferViews, draco.bufferView);
  if (!pBufferView) {
    readGltf.warnings.emplace_back("Draco bufferView index is invalid.");
    return nullptr;
  }

  const CesiumGltf::BufferView& bufferView = *pBufferView;

  CesiumGltf::Buffer* pBuffer =
      CesiumGltf::Model::getSafe(&model.buffers, bufferView.buffer);
  if (!pBuffer) {
    readGltf.warnings.emplace_back(
        "Draco bufferView has an invalid buffer index.");
    return nullptr;
  }

  CesiumGltf::Buffer& buffer = *pBuffer;

  if (bufferView.byteOffset < 0 || bufferView.byteLength < 0 ||
      bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
    readGltf.warnings.emplace_back(
        "Draco bufferView extends beyond its buffer.");
    return nullptr;
  }

  const std::span<const std::byte> data(
      buffer.cesium.data.data() + bufferView.byteOffset,
      static_cast<uint64_t>(bufferView.byteLength));

  draco::DecoderBuffer decodeBuffer;
  decodeBuffer.Init(reinterpret_cast<const char*>(data.data()), data.size());

  draco::Decoder decoder;
  draco::Mesh mesh;
  draco::StatusOr<std::unique_ptr<draco::Mesh>> result =
      decoder.DecodeMeshFromBuffer(&decodeBuffer);
  if (!result.ok()) {
    readGltf.warnings.emplace_back(
        std::string("Draco decoding failed: ") +
        result.status().error_msg_string());
    return nullptr;
  }

  return std::move(result).value();
}

template <typename TSource, typename TDestination>
void copyData(
    const TSource* pSource,
    TDestination* pDestination,
    int64_t length) {
  std::transform(pSource, pSource + length, pDestination, [](auto x) {
    return static_cast<TDestination>(x);
  });
}

template <typename T>
void copyData(const T* pSource, T* pDestination, int64_t length) {
  std::copy(pSource, pSource + length, pDestination);
}

void copyDecodedIndices(
    GltfReaderResult& readGltf,
    const CesiumGltf::MeshPrimitive& primitive,
    draco::Mesh* pMesh) {
  CESIUM_TRACE("CesiumGltfReader::copyDecodedIndices");
  CESIUM_ASSERT(readGltf.model.has_value());
  CesiumGltf::Model& model = readGltf.model.value();

  if (primitive.indices < 0) {
    return;
  }

  CesiumGltf::Accessor* pIndicesAccessor =
      CesiumGltf::Model::getSafe(&model.accessors, primitive.indices);
  if (!pIndicesAccessor) {
    readGltf.warnings.emplace_back("Primitive indices accessor ID is invalid.");
    return;
  }

  if (pIndicesAccessor->count != static_cast<int64_t>(pMesh->num_faces() * 3)) {
    readGltf.warnings.emplace_back(
        "indices accessor doesn't match with decoded Draco indices");
    pIndicesAccessor->count = static_cast<int64_t>(pMesh->num_faces() * 3);
  }

  draco::PointIndex::ValueType numPoint = pMesh->num_points();
  int32_t supposedComponentType =
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE;
  if (numPoint < static_cast<draco::PointIndex::ValueType>(
                     std::numeric_limits<uint8_t>::max())) {
    supposedComponentType = CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE;
  } else if (
      numPoint < static_cast<draco::PointIndex::ValueType>(
                     std::numeric_limits<uint16_t>::max())) {
    supposedComponentType = CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT;
  } else {
    supposedComponentType = CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
  }

  if (supposedComponentType > pIndicesAccessor->componentType) {
    pIndicesAccessor->componentType = supposedComponentType;
  }

  pIndicesAccessor->bufferView = static_cast<int32_t>(model.bufferViews.size());
  CesiumGltf::BufferView& indicesBufferView = model.bufferViews.emplace_back();

  indicesBufferView.buffer = static_cast<int32_t>(model.buffers.size());
  CesiumGltf::Buffer& indicesBuffer = model.buffers.emplace_back();

  int64_t indexBytes = pIndicesAccessor->computeByteSizeOfComponent();
  const int64_t indicesBytes = pIndicesAccessor->count * indexBytes;

  indicesBuffer.cesium.data.resize(static_cast<size_t>(indicesBytes));
  indicesBuffer.byteLength = indicesBytes;
  indicesBufferView.byteLength = indicesBytes;
  indicesBufferView.byteOffset = 0;
  indicesBufferView.target =
      CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER;
  pIndicesAccessor->type = CesiumGltf::Accessor::Type::SCALAR;
  pIndicesAccessor->byteOffset = 0;

  static_assert(sizeof(draco::PointIndex) == sizeof(uint32_t));

  const uint32_t* pSourceIndices =
      reinterpret_cast<const uint32_t*>(&pMesh->face(draco::FaceIndex(0))[0]);

  switch (pIndicesAccessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE:
    copyData(
        pSourceIndices,
        reinterpret_cast<int8_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    copyData(
        pSourceIndices,
        reinterpret_cast<uint8_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case CesiumGltf::Accessor::ComponentType::SHORT:
    copyData(
        pSourceIndices,
        reinterpret_cast<int16_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    copyData(
        pSourceIndices,
        reinterpret_cast<uint16_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
    copyData(
        pSourceIndices,
        reinterpret_cast<uint32_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    copyData(
        pSourceIndices,
        reinterpret_cast<float*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  }
}

void copyDecodedAttribute(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& /* primitive */,
    CesiumGltf::Accessor* pAccessor,
    const draco::Mesh* pMesh,
    const draco::PointAttribute* pAttribute) {
  CESIUM_TRACE("CesiumGltfReader::copyDecodedAttribute");
  CESIUM_ASSERT(readGltf.model.has_value());
  CesiumGltf::Model& model = readGltf.model.value();

  if (pAccessor->count != pMesh->num_points()) {
    readGltf.warnings.emplace_back("Attribute accessor.count doesn't match "
                                   "with number of decoded Draco vertices.");

    pAccessor->count = pMesh->num_points();
  }

  pAccessor->bufferView = static_cast<int32_t>(model.bufferViews.size());
  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();

  bufferView.buffer = static_cast<int32_t>(model.buffers.size());
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();

  const int8_t numberOfComponents = pAccessor->computeNumberOfComponents();
  const int64_t stride = static_cast<int64_t>(
      numberOfComponents * pAccessor->computeByteSizeOfComponent());
  const int64_t sizeBytes = pAccessor->count * stride;

  buffer.cesium.data.resize(static_cast<size_t>(sizeBytes));
  buffer.byteLength = sizeBytes;
  bufferView.byteLength = sizeBytes;
  bufferView.byteStride = stride;
  bufferView.byteOffset = 0;
  bufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;
  pAccessor->byteOffset = 0;

  const auto doCopy = [pMesh, pAttribute, numberOfComponents](auto pOut) {
    for (draco::PointIndex i(0); i < pMesh->num_points(); ++i) {
      const draco::AttributeValueIndex valueIndex = pAttribute->mapped_index(i);
      pAttribute->ConvertValue(valueIndex, numberOfComponents, pOut);
      pOut += pAttribute->num_components();
    }
  };

  switch (pAccessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE:
    doCopy(reinterpret_cast<int8_t*>(buffer.cesium.data.data()));
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    doCopy(reinterpret_cast<uint8_t*>(buffer.cesium.data.data()));
    break;
  case CesiumGltf::Accessor::ComponentType::SHORT:
    doCopy(reinterpret_cast<int16_t*>(buffer.cesium.data.data()));
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    doCopy(reinterpret_cast<uint16_t*>(buffer.cesium.data.data()));
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
    doCopy(reinterpret_cast<uint32_t*>(buffer.cesium.data.data()));
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    doCopy(reinterpret_cast<float*>(buffer.cesium.data.data()));
    break;
  default:
    readGltf.warnings.emplace_back(
        "Accessor uses an unknown componentType: " +
        std::to_string(int32_t(pAccessor->componentType)));
    break;
  }
}

void decodePrimitive(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& primitive,
    CesiumGltf::ExtensionKhrDracoMeshCompression& draco) {
  CESIUM_TRACE("CesiumGltfReader::decodePrimitive");
  CESIUM_ASSERT(readGltf.model.has_value());
  CesiumGltf::Model& model = readGltf.model.value();

  std::unique_ptr<draco::Mesh> pMesh =
      decodeBufferViewToDracoMesh(readGltf, primitive, draco);
  if (!pMesh) {
    return;
  }

  copyDecodedIndices(readGltf, primitive, pMesh.get());

  for (const std::pair<const std::string, int32_t>& attribute :
       draco.attributes) {
    auto primitiveAttrIt = primitive.attributes.find(attribute.first);
    if (primitiveAttrIt == primitive.attributes.end()) {
      // The primitive does not use this attribute. The
      // KHR_draco_mesh_compression spec says this shouldn't happen, so warn.
      readGltf.warnings.emplace_back(
          "Draco extension has the " + attribute.first +
          " attribute, but the primitive does not have that attribute.");
      continue;
    }

    const int32_t primitiveAttrIndex = primitiveAttrIt->second;
    CesiumGltf::Accessor* pAccessor =
        CesiumGltf::Model::getSafe(&model.accessors, primitiveAttrIndex);
    if (!pAccessor) {
      readGltf.warnings.emplace_back(
          "Primitive attribute's accessor index is invalid.");
      continue;
    }

    const int32_t dracoAttrIndex = attribute.second;
    const draco::PointAttribute* pAttribute =
        pMesh->GetAttributeByUniqueId(static_cast<uint32_t>(dracoAttrIndex));
    if (pAttribute == nullptr) {
      readGltf.warnings.emplace_back(
          "Draco attribute with unique ID " + std::to_string(dracoAttrIndex) +
          " does not exist.");
      continue;
    }

    copyDecodedAttribute(
        readGltf,
        primitive,
        pAccessor,
        pMesh.get(),
        pAttribute);
  }
}
} // namespace

void decodeDraco(CesiumGltfReader::GltfReaderResult& readGltf) {
  CESIUM_TRACE("CesiumGltfReader::decodeDraco");
  if (!readGltf.model) {
    return;
  }

  CesiumGltf::Model& model = readGltf.model.value();

  for (CesiumGltf::Mesh& mesh : model.meshes) {
    for (CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
      CesiumGltf::ExtensionKhrDracoMeshCompression* pDraco =
          primitive
              .getExtension<CesiumGltf::ExtensionKhrDracoMeshCompression>();
      if (!pDraco) {
        continue;
      }

      decodePrimitive(readGltf, primitive, *pDraco);

      // Remove the Draco extension as it no longer applies.
      primitive.extensions.erase(
          CesiumGltf::ExtensionKhrDracoMeshCompression::ExtensionName);
    }
  }

  model.removeExtensionRequired(
      CesiumGltf::ExtensionKhrDracoMeshCompression::ExtensionName);
}

} // namespace CesiumGltfReader
