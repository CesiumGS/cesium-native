#include "decodeMeshOpt.h"

#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>

#include <meshoptimizer.h>

using namespace std;
using namespace CesiumGltf;

namespace CesiumGltfReader {

namespace {

void decodeBufferView(
    Accessor& accessor,
    Buffer& destination,
    const gsl::span<const byte>& buffer,
    BufferView& bufferView,
    ExtensionBufferViewExtMeshoptCompression& meshOpt) {

  destination.byteLength =
      static_cast<int64_t>(meshOpt.byteStride * accessor.count);
  destination.cesium.data.resize(static_cast<size_t>(destination.byteLength));
  bufferView.byteLength = destination.byteLength;
  bufferView.byteStride = meshOpt.byteStride;
  bufferView.byteOffset = 0;

  if (meshOpt.mode ==
      ExtensionBufferViewExtMeshoptCompression::Mode::ATTRIBUTES) {
    if (meshopt_decodeVertexBuffer(
            destination.cesium.data.data(),
            static_cast<size_t>(accessor.count),
            static_cast<size_t>(meshOpt.byteStride),
            reinterpret_cast<const unsigned char*>(buffer.data()),
            buffer.size())) {
      return;
    }
  } else if (
      meshOpt.mode ==
      ExtensionBufferViewExtMeshoptCompression::Mode::TRIANGLES) {
    if (meshopt_decodeIndexBuffer(
            reinterpret_cast<uint16_t*>(destination.cesium.data.data()),
            static_cast<size_t>(accessor.count),
            sizeof(uint16_t),
            reinterpret_cast<const unsigned char*>(buffer.data()),
            buffer.size())) {
      return;
    }
  } else if (
      meshOpt.mode == ExtensionBufferViewExtMeshoptCompression::Mode::INDICES) {
    if (meshopt_decodeIndexSequence(
            reinterpret_cast<uint16_t*>(destination.cesium.data.data()),
            static_cast<size_t>(accessor.count),
            sizeof(uint16_t),
            reinterpret_cast<const unsigned char*>(buffer.data()),
            buffer.size())) {
      return;
    }
  }
}

void decodeAccessor(int32_t accessor, Model& model) {
  Accessor* pAccessor = Model::getSafe(&model.accessors, accessor);
  if (pAccessor) {
    BufferView* pBufferView =
        Model::getSafe(&model.bufferViews, pAccessor->bufferView);
    if (pBufferView) {
      ExtensionBufferViewExtMeshoptCompression* meshOpt =
          pBufferView->getExtension<ExtensionBufferViewExtMeshoptCompression>();
      if (meshOpt) {
        pBufferView->buffer = static_cast<int32_t>(model.buffers.size());
        Buffer* pDest = &model.buffers.emplace_back();
        Buffer* pBuffer = model.getSafe(&model.buffers, meshOpt->buffer);
        if (pBuffer) {
          decodeBufferView(
              *pAccessor,
              *pDest,
              gsl::span<const byte>(
                  pBuffer->cesium.data.data() + meshOpt->byteOffset,
                  static_cast<uint64_t>(meshOpt->byteLength)),
              *pBufferView,
              *meshOpt);
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
      for (pair<const string, int32_t>& attribute : primitive.attributes) {
        decodeAccessor(attribute.second, model);
      }
    }
  }
}
} // namespace CesiumGltfReader
