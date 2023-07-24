#include "decodeMeshOpt.h"

#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <meshoptimizer.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

using namespace CesiumGltf;

namespace CesiumGltfReader {

namespace {

template <typename T>
int decodeIndices(
    T* data,
    const gsl::span<const std::byte>& buffer,
    ExtensionBufferViewExtMeshoptCompression& meshOpt) {
  if (meshOpt.mode ==
      ExtensionBufferViewExtMeshoptCompression::Mode::TRIANGLES) {
    return meshopt_decodeIndexBuffer<T>(
        data,
        static_cast<size_t>(meshOpt.count),
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());
  } else if (
      meshOpt.mode == ExtensionBufferViewExtMeshoptCompression::Mode::INDICES) {
    return meshopt_decodeIndexSequence<T>(
        data,
        static_cast<size_t>(meshOpt.count),
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());
  }
  return -1; // invalid mode;
}

int decodeBufferView(
    void* pDest,
    const gsl::span<const std::byte>& buffer,
    ExtensionBufferViewExtMeshoptCompression& meshOpt) {
  if (meshOpt.mode ==
      ExtensionBufferViewExtMeshoptCompression::Mode::ATTRIBUTES) {
    return meshopt_decodeVertexBuffer(
        pDest,
        static_cast<size_t>(meshOpt.count),
        static_cast<size_t>(meshOpt.byteStride),
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());
  } else {
    if (meshOpt.byteStride == sizeof(std::uint16_t)) {
      return decodeIndices<std::uint16_t>(
          reinterpret_cast<std::uint16_t*>(pDest),
          buffer,
          meshOpt);
    } else if (meshOpt.byteStride == sizeof(std::uint32_t)) {
      return decodeIndices<std::uint32_t>(
          reinterpret_cast<std::uint32_t*>(pDest),
          buffer,
          meshOpt);
    } else {
      return -1;
    }
  }
}

void decodeAccessor(
    int32_t accessor,
    Model& model,
    CesiumGltfReader::GltfReaderResult& readGltf) {
  Accessor* pAccessor = Model::getSafe(&model.accessors, accessor);
  if (!pAccessor) {
    return;
  }
  BufferView* pBufferView =
      Model::getSafe(&model.bufferViews, pAccessor->bufferView);
  if (!pBufferView) {
    return;
  }
  ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
      pBufferView->getExtension<ExtensionBufferViewExtMeshoptCompression>();
  if (!pMeshOpt) {
    return;
  }
  Buffer* pBuffer = model.getSafe(&model.buffers, pMeshOpt->buffer);
  if (!pBuffer) {
    readGltf.warnings.emplace_back(
        "The ext_meshopt_compression extension has an invalid buffer "
        "index.");
    return;
  }
  if (pMeshOpt->byteOffset < 0 || pMeshOpt->byteLength < 0 ||
      pMeshOpt->byteOffset + pMeshOpt->byteLength >
          static_cast<int64_t>(pBuffer->cesium.data.size())) {
    readGltf.warnings.emplace_back(
        "The ext_meshopt_compression extension has a bufferView that "
        "extends beyond its buffer.");
    return;
  }
  Buffer* pDest = &model.buffers.emplace_back();
  pBuffer = model.getSafe(&model.buffers, pMeshOpt->buffer);
  int64_t byteLength = pMeshOpt->byteStride * pMeshOpt->count;
  pDest->byteLength = byteLength;
  pDest->cesium.data.resize(static_cast<size_t>(byteLength));
  pBufferView->byteLength = byteLength;
  pBufferView->byteStride = pMeshOpt->byteStride;
  pBufferView->byteOffset = 0;
  if (decodeBufferView(
          pDest->cesium.data.data(),
          gsl::span<const std::byte>(
              pBuffer->cesium.data.data() + pMeshOpt->byteOffset,
              static_cast<size_t>(pMeshOpt->byteLength)),
          *pMeshOpt) != 0) {
    readGltf.warnings.emplace_back(
        "The ext_meshopt_compression extension has a corrupted or "
        "incompatible meshopt compression buffer.");
    return;
  }
  pBufferView->buffer = static_cast<int32_t>(model.buffers.size() - 1);
  pBufferView->extensions.erase(
      ExtensionBufferViewExtMeshoptCompression::ExtensionName);
}
} // namespace

void decodeMeshOpt(Model& model, CesiumGltfReader::GltfReaderResult& readGltf) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      decodeAccessor(primitive.indices, model, readGltf);
      for (std::pair<const std::string, int32_t> attribute :
           primitive.attributes) {
        decodeAccessor(attribute.second, model, readGltf);
      }
    }
  }
}
} // namespace CesiumGltfReader
