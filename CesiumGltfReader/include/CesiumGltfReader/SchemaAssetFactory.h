#pragma once

#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfReader/SchemaReader.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace CesiumGltfReader {

struct SchemaAssetFactory : CesiumAsync::AssetFactory<CesiumGltf::Schema> {
  CesiumUtility::IntrusivePointer<CesiumGltf::Schema>
  createFrom(const gsl::span<const gsl::byte>& data) const override {
    SchemaReader reader{};
    CesiumJsonReader::ReadJsonResult<CesiumGltf::Schema> result =
        reader.readFromJson(data);

    // TODO: add errors & warnings to gltf results
    if (result.value.has_value()) {
      CesiumUtility::IntrusivePointer<CesiumGltf::Schema> schemaResult;

      schemaResult.emplace(*result.value);
      return schemaResult;
    }

    return nullptr;
  }
};

} // namespace CesiumGltfReader
