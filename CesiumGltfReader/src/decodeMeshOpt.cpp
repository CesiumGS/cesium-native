#include "decodeMeshOpt.h"

#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

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

void decodeFilter(
    std::byte* buffer,
    const ExtensionBufferViewExtMeshoptCompression& meshOpt) {
  if (meshOpt.filter ==
      ExtensionBufferViewExtMeshoptCompression::Filter::NONE) {
    return;
  }
  if (meshOpt.filter ==
      ExtensionBufferViewExtMeshoptCompression::Filter::OCTAHEDRAL) {
    meshopt_decodeFilterOct(
        buffer,
        static_cast<size_t>(meshOpt.count),
        static_cast<size_t>(meshOpt.byteStride));
  } else if (
      meshOpt.filter ==
      ExtensionBufferViewExtMeshoptCompression::Filter::QUATERNION) {
    meshopt_decodeFilterQuat(
        buffer,
        static_cast<size_t>(meshOpt.count),
        static_cast<size_t>(meshOpt.byteStride));
  } else if (
      meshOpt.filter ==
      ExtensionBufferViewExtMeshoptCompression::Filter::EXPONENTIAL) {
    meshopt_decodeFilterExp(
        buffer,
        static_cast<size_t>(meshOpt.count),
        static_cast<size_t>(meshOpt.byteStride));
  }
}

template <typename T>
int decodeIndices(
    T* data,
    const std::span<const std::byte>& buffer,
    const ExtensionBufferViewExtMeshoptCompression& meshOpt) {
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
    void* data,
    const std::span<const std::byte>& buffer,
    const ExtensionBufferViewExtMeshoptCompression& meshOpt) {
  if (meshOpt.mode ==
      ExtensionBufferViewExtMeshoptCompression::Mode::ATTRIBUTES) {
    return meshopt_decodeVertexBuffer(
        data,
        static_cast<size_t>(meshOpt.count),
        static_cast<size_t>(meshOpt.byteStride),
        reinterpret_cast<const unsigned char*>(buffer.data()),
        buffer.size());
  } else {
    if (meshOpt.byteStride == sizeof(std::uint16_t)) {
      return decodeIndices(
          reinterpret_cast<std::uint16_t*>(data),
          buffer,
          meshOpt);
    } else if (meshOpt.byteStride == sizeof(std::uint32_t)) {
      return decodeIndices(
          reinterpret_cast<std::uint32_t*>(data),
          buffer,
          meshOpt);
    } else {
      return -1;
    }
  }
}
} // namespace

void decodeMeshOpt(Model& model, CesiumGltfReader::GltfReaderResult& readGltf) {
  for (BufferView& bufferView : model.bufferViews) {
    const ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
        bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
    if (pMeshOpt) {
      const Buffer* pBuffer = model.getSafe(&model.buffers, pMeshOpt->buffer);
      if (!pBuffer) {
        readGltf.warnings.emplace_back(
            "The EXT_meshopt_compression extension has an invalid buffer "
            "index.");
        continue;
      }

      if (pMeshOpt->byteOffset < 0 || pMeshOpt->byteLength < 0 ||
          static_cast<size_t>(pMeshOpt->byteOffset + pMeshOpt->byteLength) >
              pBuffer->cesium.data.size()) {
        readGltf.warnings.emplace_back(
            "The EXT_meshopt_compression extension has a bufferView that "
            "extends beyond its buffer.");
        continue;
      }
      int64_t byteLength = pMeshOpt->byteStride * pMeshOpt->count;
      if (byteLength < 0) {
        readGltf.warnings.emplace_back("The EXT_meshopt_compression extension "
                                       "has a negative byte length.");
        continue;
      }

      std::vector<std::byte> data;
      data.resize(static_cast<size_t>(byteLength));
      if (decodeBufferView(
              data.data(),
              std::span<const std::byte>(
                  pBuffer->cesium.data.data() + pMeshOpt->byteOffset,
                  static_cast<size_t>(pMeshOpt->byteLength)),
              *pMeshOpt) != 0) {
        readGltf.warnings.emplace_back(
            "The EXT_meshopt_compression extension has a corrupted or "
            "incompatible meshopt compression buffer.");
        continue;
      }
      decodeFilter(data.data(), *pMeshOpt);
      Buffer& buffer = model.buffers.emplace_back();
      buffer.byteLength = byteLength;
      buffer.cesium.data = std::move(data);

      bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
      bufferView.byteOffset = 0;
      bufferView.byteLength = byteLength;
      bufferView.extensions.erase(
          ExtensionBufferViewExtMeshoptCompression::ExtensionName);
    }
  }

  model.removeExtensionRequired(
      CesiumGltf::ExtensionBufferViewExtMeshoptCompression::ExtensionName);
}
} // namespace CesiumGltfReader
