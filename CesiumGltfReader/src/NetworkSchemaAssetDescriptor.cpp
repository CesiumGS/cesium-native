#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGltfReader/NetworkSchemaAssetDescriptor.h>
#include <CesiumGltfReader/SchemaReader.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace CesiumGltfReader {

bool NetworkSchemaAssetDescriptor::operator==(
    const NetworkSchemaAssetDescriptor& rhs) const noexcept {
  return NetworkAssetDescriptor::operator==(rhs);
}

Future<ResultPointer<Schema>> NetworkSchemaAssetDescriptor::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const {
  return this->loadBytesFromNetwork(asyncSystem, pAssetAccessor)
      .thenInWorkerThread([](Result<std::vector<std::byte>>&& result) {
        if (!result.value) {
          return ResultPointer<Schema>(result.errors);
        }

        SchemaReader reader{};
        CesiumJsonReader::ReadJsonResult<CesiumGltf::Schema> jsonReadResult =
            reader.readFromJson(*result.value);

        CesiumUtility::IntrusivePointer<CesiumGltf::Schema> pSchema;
        if (result.value.has_value()) {
          pSchema.emplace(std::move(*jsonReadResult.value));
        }

        result.errors.merge(
            ErrorList{jsonReadResult.errors, jsonReadResult.warnings});

        return ResultPointer<Schema>(pSchema, std::move(result.errors));
      });
}
} // namespace CesiumGltfReader

std::size_t
std::hash<CesiumGltfReader::NetworkSchemaAssetDescriptor>::operator()(
    const CesiumGltfReader::NetworkSchemaAssetDescriptor& key) const noexcept {
  std::hash<NetworkAssetDescriptor> baseHash{};
  size_t result = baseHash(key);
  return result;
}
