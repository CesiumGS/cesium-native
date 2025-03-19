#include "dequantizeMeshData.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorSpec.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumGltf;

namespace CesiumGltfReader {

namespace {

template <typename T> float intToFloat(T t) = delete;

template <> float intToFloat(std::int8_t c) {
  return std::max(c / 127.0f, -1.0f);
}

template <> float intToFloat(std::uint8_t c) { return c / 255.0f; }

template <> float intToFloat(std::int16_t c) {
  return std::max(c / 32767.0f, -1.0f);
}

template <> float intToFloat(std::uint16_t c) { return c / 65535.0f; }

template <typename T, size_t N>
void normalizeQuantized(
    float* fPtr,
    int64_t count,
    const std::byte* bPtr,
    int64_t stride) {
  for (int i = 0; i < count; i++, bPtr += stride) {
    for (unsigned int j = 0; j < N; j++) {
      *fPtr++ = intToFloat<T>(reinterpret_cast<const T*>(bPtr)[j]);
    }
  }
}

template <typename T, size_t N>
void castQuantizedToFloat(
    float* fPtr,
    int64_t count,
    const std::byte* bPtr,
    int64_t stride) {
  for (int i = 0; i < count; i++, bPtr += stride) {
    for (unsigned int j = 0; j < N; j++) {
      *fPtr++ = static_cast<float>(reinterpret_cast<const T*>(bPtr)[j]);
    }
  }
}

template <typename T, size_t N>
void dequantizeAccessor(Model& model, Accessor& accessor) {

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

  if (static_cast<size_t>(
          pBufferView->byteOffset + accessor.byteOffset +
          accessor.count * byteStride) > pBuffer->cesium.data.size() ||
      static_cast<int64_t>(sizeof(T) * N) > byteStride) {
    return;
  }

  int64_t byteLength = accessor.count * static_cast<int64_t>(N * sizeof(float));
  if (byteLength < 0) {
    return;
  }

  std::vector<std::byte> data;
  data.resize(static_cast<size_t>(byteLength));

  const std::byte* bPtr = pBuffer->cesium.data.data() +
                          pBufferView->byteOffset + accessor.byteOffset;

  if (accessor.normalized) {
    normalizeQuantized<T, N>(
        reinterpret_cast<float*>(data.data()),
        accessor.count,
        bPtr,
        byteStride);
    for (double& d : accessor.min) {
      d = intToFloat<T>(static_cast<T>(d));
    }
    for (double& d : accessor.max) {
      d = intToFloat<T>(static_cast<T>(d));
    }
  } else {
    castQuantizedToFloat<T, N>(
        reinterpret_cast<float*>(data.data()),
        accessor.count,
        bPtr,
        byteStride);
  }
  accessor.componentType = AccessorSpec::ComponentType::FLOAT;
  accessor.byteOffset = 0;
  accessor.bufferView = static_cast<int32_t>(model.bufferViews.size());
  accessor.normalized = false;

  BufferView& bufferView = model.bufferViews.emplace_back(*pBufferView);
  bufferView.byteOffset = 0;
  bufferView.byteStride = N * sizeof(float);
  bufferView.byteLength = byteLength;
  bufferView.buffer = static_cast<int32_t>(model.buffers.size());

  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data = std::move(data);
  buffer.byteLength = byteLength;
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
      for (const std::pair<const std::string, int32_t>& attribute :
           primitive.attributes) {
        Accessor* pAccessor =
            Model::getSafe(&model.accessors, attribute.second);
        if (!pAccessor) {
          continue;
        }
        if (pAccessor->componentType == Accessor::ComponentType::FLOAT) {
          continue;
        }
        const std::string& attributeName = attribute.first;
        if (attributeName == "POSITION" || attributeName == "NORMAL" ||
            attributeName == "TANGENT" ||
            attributeName.starts_with("TEXCOORD")) {
          dequantizeAccessor(model, *pAccessor);
        }
      }
    }
  }

  model.removeExtensionRequired("KHR_mesh_quantization");
}
} // namespace CesiumGltfReader
