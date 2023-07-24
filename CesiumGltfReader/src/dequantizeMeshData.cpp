#include "dequantizeMeshData.h"

#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>

#include <meshoptimizer.h>

using namespace CesiumGltf;

namespace CesiumGltfReader {

namespace {

template <typename T> float intToFloat(T t) { return intToFloat(t); }

template <> float intToFloat(std::int8_t c) {
  return std::max(c / 127.0f, -1.0f);
}

template <> float intToFloat(std::uint8_t c) { return c / 127.0f; }

template <> float intToFloat(std::int16_t c) {
  return std::max(c / 65535.0f, -1.0f);
}

template <> float intToFloat(std::uint16_t c) { return c / 65535.0f; }

template <typename T, size_t N>
void dequantizeFloat(
    float* fPtr,
    int64_t count,
    std::byte* bPtr,
    int64_t stride) {
  for (int i = 0; i < count; i++, bPtr += stride) {
    for (unsigned int j = 0; j < N; j++) {
      *fPtr++ = intToFloat<T>(reinterpret_cast<T*>(bPtr)[j]);
    }
  }
}

template <typename T, size_t N>
void dequantizeAccessor(Model& model, Accessor& accessor) {

  accessor.componentType = AccessorSpec::ComponentType::FLOAT;
  accessor.byteOffset = 0;
  for (double& d : accessor.min) {
    d = intToFloat<T>(static_cast<T>(d));
  }
  for (double& d : accessor.max) {
    d = intToFloat<T>(static_cast<T>(d));
  }

  BufferView* pBufferView =
      Model::getSafe(&model.bufferViews, accessor.bufferView);

  if (!pBufferView) {
    return;
  }

  Buffer* pBuffer = Model::getSafe(&model.buffers, pBufferView->buffer);
  if (!pBuffer) {
    return;
  }

  int64_t byteStride;
  if (pBufferView->byteStride) {
    byteStride = *pBufferView->byteStride;
  } else {
    byteStride = accessor.computeByteStride(model);
  }

  int64_t byteLength = accessor.count * N * sizeof(float);
  std::vector<std::byte> buffer;
  buffer.resize(byteLength);

  dequantizeFloat<T, N>(
      reinterpret_cast<float*>(buffer.data()),
      accessor.count,
      pBuffer->cesium.data.data() + pBufferView->byteOffset +
          accessor.byteOffset,
      byteStride);

  pBufferView->byteOffset = 0;
  pBufferView->byteStride = N * sizeof(float);
  pBufferView->byteLength = byteLength;

  pBuffer->cesium.data = std::move(buffer);
  pBuffer->byteLength = byteLength;
}

template <size_t N> void dequantizeAccessor(Model& model, Accessor& accessor) {
  switch (accessor.componentType) {
  case Accessor::ComponentType::BYTE:
    dequantizeAccessor<std::int8_t, N>(model, accessor);
    break;
  case Accessor::ComponentType::UNSIGNED_BYTE:
    dequantizeAccessor<std::uint8_t, N>(model, accessor);
    break;
  case Accessor::ComponentType::SHORT:
    dequantizeAccessor<std::int16_t, N>(model, accessor);
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    dequantizeAccessor<std::uint16_t, N>(model, accessor);
    break;
  }
}

void dequantizeAccessor(Model& model, Accessor& accessor) {
  int8_t numberOfComponents = accessor.computeNumberOfComponents();
  switch (numberOfComponents) {
  case 2:
    dequantizeAccessor<2>(model, accessor);
    break;
  case 3:
    dequantizeAccessor<3>(model, accessor);
    break;
  case 4:
    dequantizeAccessor<4>(model, accessor);
    break;
  }
}
} // namespace

void dequantizeMeshData(Model& model) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      for (std::pair<const std::string, int32_t> attribute :
           primitive.attributes) {
        Accessor* pAccessor =
            Model::getSafe(&model.accessors, attribute.second);
        if (pAccessor) {
          dequantizeAccessor(model, *pAccessor);
        }
      }
    }
  }
}
} // namespace CesiumGltfReader
