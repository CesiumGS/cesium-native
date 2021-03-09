#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/Model.h"
#include "catch2/catch.hpp"
#include <glm/vec3.hpp>

TEST_CASE("AccessorView construct and read example") {
  auto anyOldFunctionToGetAModel = []() {
    CesiumGltf::Model model;

    CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
    accessor.type = CesiumGltf::Accessor::Type::VEC3;
    accessor.count = 1;

    CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = accessor.count * int64_t(sizeof(float)) * 3;

    CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
    buffer.byteLength = bufferView.byteLength;
    buffer.cesium.data.resize(size_t(buffer.byteLength));

    float* p = reinterpret_cast<float*>(buffer.cesium.data.data());
    p[0] = 1.0f;
    p[1] = 2.0f;
    p[2] = 3.0f;

    return model;
  };

  //! [createFromAccessorAndRead]
  CesiumGltf::Model model = anyOldFunctionToGetAModel();
  CesiumGltf::AccessorView<glm::vec3> positions(model, 0);
  glm::vec3 firstPosition = positions[0];
  //! [createFromAccessorAndRead]

  CHECK(firstPosition == glm::vec3(1.0f, 2.0f, 3.0f));
}
