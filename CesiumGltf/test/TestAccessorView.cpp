#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/Model.h"

#include <catch2/catch.hpp>
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

TEST_CASE("Create AccessorView of unknown type with lambda") {
  using namespace CesiumGltf;

  Model model;

  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(4);
  buffer.cesium.data[0] = std::byte(1);
  buffer.cesium.data[1] = std::byte(2);
  buffer.cesium.data[2] = std::byte(3);
  buffer.cesium.data[3] = std::byte(4);
  buffer.byteLength = int64_t(buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = 0;
  accessor.count = 1;

  accessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
  createAccessorView(model, accessor, [](const auto& accessorView) {
    CHECK(accessorView.status() == AccessorViewStatus::Valid);

    // While this generic lambda will be instantiated for all possible types,
    // it should only be _called_ for the actual type (UNSIGNED_INT).
    using AccessorType =
        std::remove_cv_t<std::remove_reference_t<decltype(accessorView)>>;
    CHECK(std::is_same_v<
          AccessorType,
          AccessorView<AccessorTypes::SCALAR<uint32_t>>>);

    CHECK(int64_t(accessorView[0].value[0]) == int64_t(0x04030201));
  });

  accessor.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
  createAccessorView(model, accessor, [](const auto& accessorView) {
    CHECK(accessorView.status() == AccessorViewStatus::Valid);

    // While this generic lambda will be instantiated for all possible types,
    // it should only be _called_ for the actual type (UNSIGNED_SHORT).
    using AccessorType =
        std::remove_cv_t<std::remove_reference_t<decltype(accessorView)>>;
    CHECK(std::is_same_v<
          AccessorType,
          AccessorView<AccessorTypes::SCALAR<uint16_t>>>);

    CHECK(int64_t(accessorView[0].value[0]) == int64_t(0x0201));
  });
}
