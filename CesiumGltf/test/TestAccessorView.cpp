#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Model.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_float3.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

using namespace CesiumGltf;

TEST_CASE("AccessorView construct and read example") {
  auto anyOldFunctionToGetAModel = []() {
    Model model;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = 1;

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = accessor.count * int64_t(sizeof(float)) * 3;

    Buffer& buffer = model.buffers.emplace_back();
    buffer.byteLength = bufferView.byteLength;
    buffer.cesium.data.resize(size_t(buffer.byteLength));

    float* p = reinterpret_cast<float*>(buffer.cesium.data.data());
    p[0] = 1.0f;
    p[1] = 2.0f;
    p[2] = 3.0f;

    return model;
  };

  //! [createFromAccessorAndRead]
  Model model = anyOldFunctionToGetAModel();
  AccessorView<glm::vec3> positions(model, 0);
  glm::vec3 firstPosition = positions[0];
  //! [createFromAccessorAndRead]

  CHECK(firstPosition == glm::vec3(1.0f, 2.0f, 3.0f));

  CHECK(positions.size() == 1);
  CHECK(positions.status() == AccessorViewStatus::Valid);
  CHECK(positions.stride() == 12);
  CHECK(positions.offset() == 0);
  CHECK(positions.data() == model.buffers[0].cesium.data.data());
}

TEST_CASE("Create AccessorView of unknown type with lambda") {
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
