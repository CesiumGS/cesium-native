#include <CesiumGltf/GenericPropertyTableViewVistor.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTablePropertyView.h>
#include <CesiumGltf/PropertyTableView.h>

#include <catch2/catch.hpp>

using namespace CesiumGltf;

namespace {

template <typename T>
void addBufferToModel(Model& model, const std::vector<T>& values) {
  Buffer& valueBuffer = model.buffers.emplace_back();
  valueBuffer.cesium.data.resize(values.size() * sizeof(T));
  valueBuffer.byteLength = static_cast<int64_t>(valueBuffer.cesium.data.size());
  std::memcpy(
      valueBuffer.cesium.data.data(),
      values.data(),
      valueBuffer.cesium.data.size());

  BufferView& valueBufferView = model.bufferViews.emplace_back();
  valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  valueBufferView.byteOffset = 0;
  valueBufferView.byteLength = valueBuffer.byteLength;
}

} // namespace

TEST_CASE("Can use virtual dispatch of PropertyTablePropertyViews") {
  Model model;
  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;

  GenericPropertyTableViewVisitor dispatcher(
      [&values, &invokedCallbackCount](
          //const std::string& /*propertyName*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<uint32_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
          REQUIRE(propertyValue.size() > 0);

          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<uint32_t>(propertyValue.getRaw(i)) ==
                expectedValue);
            REQUIRE(propertyValue.get(i) == expectedValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  view.getPropertyViewDynamic(
      view.findProperty("TestClassProperty"),
      dispatcher);

  REQUIRE(invokedCallbackCount == 1);
}
