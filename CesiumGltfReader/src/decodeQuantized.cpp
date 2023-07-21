#include "decodeQuantized.h"

#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>

#include <meshoptimizer.h>

using namespace CesiumGltf;

namespace CesiumGltfReader {

namespace {
template <typename T, size_t N>
using xVec = AccessorView<glm::vec<N, T, glm::defaultp>>;

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
void unquantizeFloat(float* fPtr, xVec<T, N>& quantizedView) {
  for (int i = 0; i < quantizedView.size(); i++) {
    const auto& q = quantizedView[i];
    for (unsigned int j = 0; j < q.length(); j++) {
      *fPtr++ = intToFloat<T>(q[j]);
    }
  }
}

template <typename T, size_t N>
void decodeAccessor(Model& model, Accessor& accessor) {

  xVec<T, N> quantizedView(model, accessor);
  accessor.componentType = AccessorSpec::ComponentType::FLOAT;
  accessor.byteOffset = 0;
  accessor.count = accessor.count;

  for (double& d : accessor.min) {
    d = intToFloat<T>(static_cast<T>(d));
  }
  for (double& d : accessor.max) {
    d = intToFloat<T>(static_cast<T>(d));
  }

  BufferView* pBufferView =
      Model::getSafe(&model.bufferViews, accessor.bufferView);
  if (!pBufferView) {
    accessor.bufferView = static_cast<int32_t>(model.bufferViews.size());
    pBufferView = &model.bufferViews.emplace_back();
  }
  pBufferView->byteOffset = 0;
  pBufferView->byteStride = N * sizeof(float);
  pBufferView->byteLength = accessor.count * *pBufferView->byteStride;
  pBufferView->buffer = static_cast<int32_t>(model.buffers.size());

  Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = pBufferView->byteLength;
  buffer.cesium.data.resize(static_cast<size_t>(buffer.byteLength));

  unquantizeFloat<T>(
      reinterpret_cast<float*>(buffer.cesium.data.data()),
      quantizedView);
}

template <size_t N> void decodeAccessor(Model& model, Accessor& accessor) {
  switch (accessor.componentType) {
  case Accessor::ComponentType::BYTE:
    decodeAccessor<std::int8_t, N>(model, accessor);
    break;
  case Accessor::ComponentType::UNSIGNED_BYTE:
    decodeAccessor<std::uint8_t, N>(model, accessor);
    break;
  case Accessor::ComponentType::SHORT:
    decodeAccessor<std::int16_t, N>(model, accessor);
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    decodeAccessor<std::uint16_t, N>(model, accessor);
    break;
  }
}

void decodeAccessor(Model& model, Accessor& accessor) {
  int8_t numberOfComponents = accessor.computeNumberOfComponents();
  switch (numberOfComponents) {
  case 2:
    decodeAccessor<2>(model, accessor);
    break;
  case 3:
    decodeAccessor<3>(model, accessor);
    break;
  case 4:
    decodeAccessor<4>(model, accessor);
    break;
  }
}
} // namespace

void decodeQuantized(Model& model) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      for (std::pair<const std::string, int32_t>& attribute :
           primitive.attributes) {
        Accessor* pAccessor =
            Model::getSafe(&model.accessors, attribute.second);
        if (pAccessor) {
          decodeAccessor(model, *pAccessor);
        }
      }
    }
  }
}
} // namespace CesiumGltfReader
