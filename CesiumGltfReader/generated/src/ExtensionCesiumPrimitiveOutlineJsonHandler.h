// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <CesiumGltf/ExtensionCesiumPrimitiveOutline.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>

namespace CesiumJsonReader {
class JsonReaderOptions;
} // namespace CesiumJsonReader

namespace CesiumGltfReader {
class ExtensionCesiumPrimitiveOutlineJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler,
      public CesiumJsonReader::IExtensionJsonHandler {
public:
  using ValueType = CesiumGltf::ExtensionCesiumPrimitiveOutline;

  static constexpr const char* ExtensionName = "CESIUM_primitive_outline";

  explicit ExtensionCesiumPrimitiveOutlineJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& options) noexcept;
  void reset(
      IJsonHandler* pParentHandler,
      CesiumGltf::ExtensionCesiumPrimitiveOutline* pObject);

  IJsonHandler* readObjectKey(const std::string_view& str) override;

  void reset(
      IJsonHandler* pParentHandler,
      CesiumUtility::ExtensibleObject& o,
      const std::string_view& extensionName) override;

  IJsonHandler& getHandler() override { return *this; }

protected:
  IJsonHandler* readObjectKeyExtensionCesiumPrimitiveOutline(
      const std::string& objectType,
      const std::string_view& str,
      CesiumGltf::ExtensionCesiumPrimitiveOutline& o);

private:
  CesiumGltf::ExtensionCesiumPrimitiveOutline* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _indices;
};
} // namespace CesiumGltfReader
