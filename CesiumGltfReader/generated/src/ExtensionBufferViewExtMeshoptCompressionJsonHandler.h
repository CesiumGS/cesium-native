// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumJsonReader {
class JsonReaderOptions;
} // namespace CesiumJsonReader

namespace CesiumGltfReader {
class ExtensionBufferViewExtMeshoptCompressionJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler,
      public CesiumJsonReader::IExtensionJsonHandler {
public:
  using ValueType = CesiumGltf::ExtensionBufferViewExtMeshoptCompression;

  static constexpr const char* ExtensionName = "EXT_meshopt_compression";

  explicit ExtensionBufferViewExtMeshoptCompressionJsonHandler(
      const CesiumJsonReader::JsonReaderOptions& options) noexcept;
  void reset(
      IJsonHandler* pParentHandler,
      CesiumGltf::ExtensionBufferViewExtMeshoptCompression* pObject);

  IJsonHandler* readObjectKey(const std::string_view& str) override;

  void reset(
      IJsonHandler* pParentHandler,
      CesiumUtility::ExtensibleObject& o,
      const std::string_view& extensionName) override;

  IJsonHandler& getHandler() override { return *this; }

protected:
  IJsonHandler* readObjectKeyExtensionBufferViewExtMeshoptCompression(
      const std::string& objectType,
      const std::string_view& str,
      CesiumGltf::ExtensionBufferViewExtMeshoptCompression& o);

private:
  CesiumGltf::ExtensionBufferViewExtMeshoptCompression* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _buffer;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _byteOffset;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _byteLength;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _byteStride;
  CesiumJsonReader::IntegerJsonHandler<int64_t> _count;
  CesiumJsonReader::StringJsonHandler _mode;
  CesiumJsonReader::StringJsonHandler _filter;
};
} // namespace CesiumGltfReader
