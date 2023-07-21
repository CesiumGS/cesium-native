#include "decodeMeshOpt.h"

#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>

#include <meshoptimizer.h>

using namespace CesiumGltf;

namespace CesiumGltfReader {

namespace {

template <typename T>
int decodeIndices(
    void* data,
    const gsl::span<const std::byte>& buffer,
    ExtensionBufferViewExtMeshoptCompression& meshOpt) {
  if (meshOpt.mode ==
      ExtensionBufferViewExtMeshoptCompression::Mode::TRIANGLES) {
    return meshopt_decodeIndexBuffer<T>(
        reinterpret_cast<T*>(data),
        static_cast<size_t>(meshOpt.count),
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());
  } else if (
      meshOpt.mode == ExtensionBufferViewExtMeshoptCompression::Mode::INDICES) {
    return meshopt_decodeIndexSequence<T>(
        reinterpret_cast<T*>(data),
        static_cast<size_t>(meshOpt.count),
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());
  }
  return -1;
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
    if (meshOpt.count > std::numeric_limits<std::uint16_t>::max()) {
      return decodeIndices<std::uint32_t>(pDest, buffer, meshOpt);
    } else {
      return decodeIndices<std::uint16_t>(pDest, buffer, meshOpt);
    }
  }
}

void decodeAccessor(int32_t accessor, Model& model) {
  Accessor* pAccessor = Model::getSafe(&model.accessors, accessor);
  if (pAccessor) {
    BufferView* pBufferView =
        Model::getSafe(&model.bufferViews, pAccessor->bufferView);
    if (pBufferView) {
      ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
          pBufferView->getExtension<ExtensionBufferViewExtMeshoptCompression>();
      if (pMeshOpt) {
        Buffer* pBuffer = model.getSafe(&model.buffers, pMeshOpt->buffer);
        if (pBuffer) {
          Buffer* pDest = &model.buffers.emplace_back();
          pBuffer = model.getSafe(&model.buffers, pMeshOpt->buffer);
          pDest->byteLength =
              static_cast<int64_t>(pMeshOpt->byteStride * pMeshOpt->count);
          pDest->cesium.data.resize(static_cast<size_t>(pDest->byteLength));
          pBufferView->byteLength = pDest->byteLength;
          pBufferView->byteStride = pMeshOpt->byteStride;
          pBufferView->byteOffset = 0;
          if (decodeBufferView(
                  pDest->cesium.data.data(),
                  gsl::span<const std::byte>(
                      pBuffer->cesium.data.data() + pMeshOpt->byteOffset,
                      static_cast<size_t>(pMeshOpt->byteLength)),
                  *pMeshOpt) == 0) {
            pBufferView->buffer =
                static_cast<int32_t>(model.buffers.size() - 1);
            pBufferView->extensions.erase(
                ExtensionBufferViewExtMeshoptCompression::ExtensionName);
          }
        }
      }
    }
  }
}
} // namespace

void decodeMeshOpt(Model& model) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      decodeAccessor(primitive.indices, model);
      for (std::pair<const std::string, int32_t> attribute :
           primitive.attributes) {
        decodeAccessor(attribute.second, model);
      }
    }
  }
}
} // namespace CesiumGltfReader
