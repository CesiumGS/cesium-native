#include "convertPrimitiveModes.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/VertexAttributeSemantics.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Tracing.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>

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
      CesiumGltf::MeshPrimitive& primitive) {
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

  int64_t numIndices = 0;
  int32_t componentType = 0;

private:
  CesiumGltf::IndexAccessorType _accessorView;
};

void replacePrimitiveIndices(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    int32_t componentType,
    std::vector<std::byte>&& data) {
  // Replace the original indices on the mesh primitive.
  primitive.indices = int32_t(model.accessors.size());

  CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
  accessor.type = CesiumGltf::Accessor::Type::SCALAR;
  accessor.componentType = componentType;
  accessor.count = int64_t(
      data.size() /
      CesiumGltf::Accessor::computeByteSizeOfComponent(componentType));
  accessor.bufferView = int32_t(model.bufferViews.size());

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.byteLength = int64_t(data.size());
  bufferView.buffer = int32_t(model.buffers.size());

  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = bufferView.byteLength;
  buffer.cesium.data = std::move(data);
}

template <typename T>
std::vector<T> convertLineLoopIndices(const PrimitiveIndicesView& indicesView) {
  int64_t numIndices = indicesView.numIndices;

  // Example: Given a line loop with three indices A-B-C, its segments are
  // A-B, B-C, C-A, requiring six indices.
  std::vector<T> data;
  data.reserve(2 * numIndices);

  for (int64_t i = 0; i < numIndices - 1; ++i) {
    data.push_back(static_cast<T>(indicesView[i]));
    data.push_back(static_cast<T>(indicesView[i + 1]));
  }

  // Complete the loop once we reach the last index.
  data.push_back(static_cast<T>(indicesView[numIndices - 1]));
  data.push_back(static_cast<T>(indicesView[0]));

  return data;
}

void convertLineLoop(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive) {
  PrimitiveIndicesView indicesView(model, primitive);
  if (indicesView.numIndices == 0) {
    return;
  }
  std::vector<std::byte> data;

  switch (indicesView.componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
    std::vector<uint8_t> indices = convertLineLoopIndices<uint8_t>(indicesView);
    data.resize(indices.size() * sizeof(uint8_t));
    std::memcpy(data.data(), indices.data(), data.size());
  } break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
    std::vector<uint16_t> indices =
        convertLineLoopIndices<uint16_t>(indicesView);
    data.resize(indices.size() * sizeof(uint16_t));
    std::memcpy(data.data(), indices.data(), data.size());
  } break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT: {
    std::vector<uint32_t> indices =
        convertLineLoopIndices<uint32_t>(indicesView);
    data.resize(indices.size() * sizeof(uint32_t));
    std::memcpy(data.data(), indices.data(), data.size());
  } break;
  default:
    return;
  }

  replacePrimitiveIndices(
      model,
      primitive,
      indicesView.componentType,
      std::move(data));
  primitive.mode = CesiumGltf::MeshPrimitive::Mode::LINES;
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
          convertLineLoop(model, primitive);
        }
        break;
      case CesiumGltf::MeshPrimitive::Mode::LINE_STRIP:
        if (options.convertLineStrip) {
        }
        break;
      case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
        if (options.convertTriangleStrip) {
        }
        break;
      case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
        if (options.convertTriangleFan) {
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
