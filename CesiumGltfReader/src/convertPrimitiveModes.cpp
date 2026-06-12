#include "convertPrimitiveModes.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Tracing.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <variant>

namespace CesiumGltfReader {

namespace {
/**
 * @brief Finds the best fitting constant for
 * CesiumGltf::Accessor::ComponentType for the given
 * CesiumGltf::IndexAccessorType, using the number of vertices in the primitive
 * as a fallback.
 */
int32_t getBestFittingComponentType(
    const CesiumGltf::IndexAccessorType& indicesView,
    int64_t numVertices) {
  int64_t maximumIndex =
      std::visit(CesiumGltf::MaxIndexValueFromAccessor{}, indicesView);
  if (maximumIndex < 0) {
    maximumIndex = numVertices;
  }

  if (maximumIndex <= std::numeric_limits<uint8_t>::max()) {
    return CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE;
  }
  if (maximumIndex <= std::numeric_limits<uint16_t>::max()) {
    return CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT;
  }
  if (maximumIndex <= std::numeric_limits<uint32_t>::max()) {
    return CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
  }

  return -1;
}

/**
 * A view on the indices of a given primitive. Handles when the primitive does
 * not specify an index accessor.
 */
struct PrimitiveIndicesView {
public:
  PrimitiveIndicesView(
      CesiumGltf::Model& model,
      CesiumGltf::MeshPrimitive& primitive)
      : numIndices(0),
        componentType(0),
        primitiveMode(primitive.mode),
        _accessorView() {
    const CesiumGltf::Accessor* pAccessor =
        model.getSafe(&model.accessors, primitive.indices);

    if (pAccessor) {
      this->_accessorView = CesiumGltf::getIndexAccessorView(model, primitive);
      this->numIndices =
          std::visit(CesiumGltf::NumIndicesFromAccessor{}, this->_accessorView);
    } else {
      CesiumGltf::PositionAccessorType positionView =
          CesiumGltf::getPositionAccessorView(model, primitive);
      this->numIndices = positionView.size();
    }

    this->componentType =
        getBestFittingComponentType(this->_accessorView, this->numIndices);
  }

  int64_t operator[](int64_t elementIndex) const {
    int64_t index = std::visit(
        CesiumGltf::IndexFromAccessor{elementIndex},
        this->_accessorView);
    return index >= 0 ? index : elementIndex;
  }

  int64_t numIndices;
  int32_t componentType;
  int32_t primitiveMode;

private:
  CesiumGltf::IndexAccessorType _accessorView;
};

template <typename T>
std::vector<T> convertLineLoopIndices(const PrimitiveIndicesView& indicesView) {
  int64_t numIndices = indicesView.numIndices;

  // Example: Given a line loop with three indices A-B-C, its segments are
  // A-B, B-C, C-A, requiring six indices.
  std::vector<T> data;
  data.reserve(2 * size_t(numIndices));

  for (int64_t i = 0; i < numIndices - 1; ++i) {
    data.push_back(static_cast<T>(indicesView[i]));
    data.push_back(static_cast<T>(indicesView[i + 1]));
  }

  // Complete the loop once we reach the last index.
  data.push_back(static_cast<T>(indicesView[numIndices - 1]));
  data.push_back(static_cast<T>(indicesView[0]));

  return data;
}

template <typename T>
std::vector<T>
convertLineStripIndices(const PrimitiveIndicesView& indicesView) {
  int64_t numIndices = indicesView.numIndices;
  // Example: Given a line strip with four indices A-B-C-D, its segments are
  // A-B, B-C, C-D, requiring six indices.
  std::vector<T> data;
  data.reserve(2 * size_t(numIndices - 1));

  for (int64_t i = 0; i < numIndices - 1; ++i) {
    data.push_back(static_cast<T>(indicesView[i]));
    data.push_back(static_cast<T>(indicesView[i + 1]));
  }

  return data;
}

template <typename T>
std::vector<T>
convertTriangleStripIndices(const PrimitiveIndicesView& indicesView) {
  int64_t numIndices = indicesView.numIndices;
  // After the first two indices, every index corresponds to a new triangle.
  std::vector<T> data;
  data.reserve(3 * size_t(numIndices - 2));
  for (int64_t i = 0; i < numIndices - 2; ++i) {
    if (i % 2) {
      data.push_back(static_cast<T>(indicesView[i + 2]));
      data.push_back(static_cast<T>(indicesView[i + 1]));
      data.push_back(static_cast<T>(indicesView[i]));
    } else {
      data.push_back(static_cast<T>(indicesView[i]));
      data.push_back(static_cast<T>(indicesView[i + 1]));
      data.push_back(static_cast<T>(indicesView[i + 2]));
    }
  }
  return data;
}

template <typename T>
std::vector<T>
convertTriangleFanIndices(const PrimitiveIndicesView& indicesView) {
  int64_t numIndices = indicesView.numIndices;
  // After the first two indices, every index corresponds to a new triangle.
  std::vector<T> data;
  data.reserve(3 * size_t(numIndices - 2));
  for (int64_t i = 2; i < numIndices; ++i) {
    data.push_back(static_cast<T>(indicesView[0]));
    data.push_back(static_cast<T>(indicesView[i - 1]));
    data.push_back(static_cast<T>(indicesView[i]));
  }
  return data;
}

template <typename T>
std::vector<std::byte>
generateNewIndicesData(const PrimitiveIndicesView& indicesView) {
  std::vector<T> indices;
  switch (indicesView.primitiveMode) {
  case CesiumGltf::MeshPrimitive::Mode::LINE_LOOP:
    indices = convertLineLoopIndices<T>(indicesView);
    break;
  case CesiumGltf::MeshPrimitive::Mode::LINE_STRIP:
    indices = convertLineStripIndices<T>(indicesView);
    break;
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
    indices = convertTriangleStripIndices<T>(indicesView);
    break;
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
    indices = convertTriangleFanIndices<T>(indicesView);
    break;
  default:
    break;
  }
  CESIUM_ASSERT(indices.size() > 0);

  std::vector<std::byte> data(indices.size() * sizeof(T));
  std::memcpy(data.data(), indices.data(), data.size());
  return data;
}

void replacePrimitiveIndices(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    int32_t componentType,
    std::vector<std::byte>&& data) {
  // Switch the primitive mode.
  switch (primitive.mode) {
  case CesiumGltf::MeshPrimitive::Mode::LINE_LOOP:
  case CesiumGltf::MeshPrimitive::Mode::LINE_STRIP:
    primitive.mode = CesiumGltf::MeshPrimitive::Mode::LINES;
    break;
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
    primitive.mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
    break;
  default:
    CESIUM_ASSERT(false);
    return;
  }

  // Replace the original indices on the mesh primitive.
  primitive.indices = int32_t(model.accessors.size());

  CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
  accessor.type = CesiumGltf::Accessor::Type::SCALAR;
  accessor.componentType = componentType;
  accessor.count =
      int64_t(data.size()) /
      CesiumGltf::Accessor::computeByteSizeOfComponent(componentType);
  accessor.bufferView = int32_t(model.bufferViews.size());

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.byteLength = int64_t(data.size());
  bufferView.buffer = int32_t(model.buffers.size());

  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = bufferView.byteLength;
  buffer.cesium.data = std::move(data);
}

void convertPrimitiveMode(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive) {
  PrimitiveIndicesView indicesView(model, primitive);
  if (indicesView.numIndices == 0) {
    return;
  }

  std::vector<std::byte> indicesData;
  switch (indicesView.componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
    indicesData = generateNewIndicesData<uint8_t>(indicesView);
    break;
  }
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
    indicesData = generateNewIndicesData<uint16_t>(indicesView);
    break;
  }
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT: {
    indicesData = generateNewIndicesData<uint32_t>(indicesView);
    break;
  }
  default:
    return;
  }

  replacePrimitiveIndices(
      model,
      primitive,
      indicesView.componentType,
      std::move(indicesData));
}

} // namespace

void convertPrimitiveModes(
    CesiumGltf::Model& model,
    const MeshPrimitiveModeOptions& options) {
  CESIUM_TRACE("CesiumGltfReader::convertPrimitiveModes");

  for (CesiumGltf::Mesh& mesh : model.meshes) {
    for (CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
      switch (primitive.mode) {
      case CesiumGltf::MeshPrimitive::Mode::LINE_LOOP:
        if (options.convertLineLoop) {
          convertPrimitiveMode(model, primitive);
        }
        break;
      case CesiumGltf::MeshPrimitive::Mode::LINE_STRIP:
        if (options.convertLineStrip) {
          convertPrimitiveMode(model, primitive);
        }
        break;
      case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
        if (options.convertTriangleStrip) {
          convertPrimitiveMode(model, primitive);
        }
        break;
      case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
        if (options.convertTriangleFan) {
          convertPrimitiveMode(model, primitive);
        }
        break;
      default:
        break;
      }
    }
  }
}

bool requiresPrimitiveModeConversion(const MeshPrimitiveModeOptions& options) {
  return options.convertLineLoop || options.convertLineStrip ||
         options.convertTriangleFan || options.convertTriangleStrip;
}

} // namespace CesiumGltfReader
