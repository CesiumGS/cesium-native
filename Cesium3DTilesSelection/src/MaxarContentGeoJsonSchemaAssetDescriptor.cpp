#include <Cesium3DTilesReader/ExtensionSchemaMaxarContentGeoJsonReader.h>
#include <Cesium3DTilesSelection/MaxarContentGeoJsonSchemaAssetDescriptor.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumGltf/Schema.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GltfConverter.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

bool MaxarContentGeoJsonSchemaAssetDescriptor::operator==(
    const MaxarContentGeoJsonSchemaAssetDescriptor& rhs) const noexcept {
  return NetworkAssetDescriptor::operator==(rhs);
}

Future<ResultPointer<Schema>> MaxarContentGeoJsonSchemaAssetDescriptor::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) const {
  return this->loadBytesFromNetwork(asyncSystem, pAssetAccessor)
      .thenInWorkerThread([](Result<std::vector<std::byte>>&& result) {
        if (!result.value) {
          return ResultPointer<Schema>(std::move(result.errors));
        }

        Cesium3DTilesReader::ExtensionSchemaMaxarContentGeoJsonReader reader;
        CesiumJsonReader::ReadJsonResult<
            Cesium3DTiles::ExtensionSchemaMaxarContentGeoJson>
            readResult = reader.readFromJson(*result.value);

        ErrorList errors = std::move(result.errors);
        errors.merge(ErrorList{
            std::move(readResult.errors),
            std::move(readResult.warnings)});
        if (!readResult.value || errors) {
          if (!errors) {
            errors.emplaceError(
                "The MAXAR_content_geojson schema did not contain a schema "
                "object.");
          }
          return ResultPointer<Schema>(std::move(errors));
        }

        CesiumVectorData::ConvertSchemaResult converted =
            CesiumVectorData::GltfConverter::convertSchema(*readResult.value);
        if (!converted.pValue) {
          converted.errors.emplaceError(
              "The MAXAR_content_geojson schema could not be converted to a "
              "glTF schema.");
          return ResultPointer<Schema>(std::move(converted.errors));
        }

        errors.merge(std::move(converted.errors));
        return ResultPointer<Schema>(converted.pValue, std::move(errors));
      });
}

} // namespace Cesium3DTilesSelection

std::size_t
std::hash<Cesium3DTilesSelection::MaxarContentGeoJsonSchemaAssetDescriptor>::
operator()(
    const Cesium3DTilesSelection::MaxarContentGeoJsonSchemaAssetDescriptor& key)
    const noexcept {
  std::hash<CesiumAsync::NetworkAssetDescriptor> baseHash{};
  return baseHash(key);
}
