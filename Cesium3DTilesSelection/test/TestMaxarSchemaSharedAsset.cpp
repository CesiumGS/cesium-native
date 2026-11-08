#include "SimplePrepareRendererResource.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentOptions.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Schema.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {

std::vector<std::byte> toBytes(std::string_view text) {
  return std::vector<std::byte>(
      reinterpret_cast<const std::byte*>(text.data()),
      reinterpret_cast<const std::byte*>(text.data() + text.size()));
}

std::shared_ptr<SimpleAssetRequest> makeRequest(
    const std::string& url,
    uint16_t statusCode,
    std::string_view body) {
  std::unique_ptr<SimpleAssetResponse> pResponse =
      std::make_unique<SimpleAssetResponse>(
          statusCode,
          "application/json",
          HttpHeaders{},
          toBytes(body));
  return std::make_shared<SimpleAssetRequest>(
      "GET",
      url,
      HttpHeaders{},
      std::move(pResponse));
}

class CountingAssetAccessor : public SimpleAssetAccessor {
public:
  explicit CountingAssetAccessor(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>&& requests)
      : SimpleAssetAccessor(std::move(requests)) {}

  std::map<std::string, int32_t> requestCounts;

  Future<std::shared_ptr<IAssetRequest>>
  get(const AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers) override {
    ++this->requestCounts[url];
    return SimpleAssetAccessor::get(asyncSystem, url, headers);
  }
};

const char* schemaJson = R"({
  "name": "Ships",
  "semantic": "ships",
  "geometry": { "type": "LineString", "dimensions": 3 },
  "properties": [
    { "id": "FULL_NAME", "type": "String", "required": false }
  ]
})";

const char* geoJson = R"({
  "type": "Feature",
  "properties": { "FULL_NAME": "Harbor" },
  "geometry": {
    "type": "LineString",
    "coordinates": [[-122.39, 37.61, 0.0], [-122.38, 37.62, 1.0]]
  }
})";

std::string tilesetWithSchemaUri(
    const std::string& schemaUri,
    const std::string& rootExtraJson) {
  return std::string(R"({
    "asset": { "version": "1.1" },
    "geometricError": 100,
    "schema": {
      "id": "tilesetSchema",
      "classes": { "tileset": { "properties": {} } }
    },
    "metadata": {
      "class": "tileset",
      "extensions": {
        "MAXAR_content_geojson": { "propertiesSchemaUri": ")") +
         schemaUri + R"(" }
      }
    },
    "extensionsUsed": ["MAXAR_content_geojson"],
    "extensionsRequired": ["MAXAR_content_geojson"],
    "root": {
      "boundingVolume": {
        "region": [-1.0, -0.5, 1.0, 0.5, 0.0, 10.0]
      },
      "geometricError": 0)" +
         rootExtraJson + R"(
    }
  })";
}

TilesetExternals
makeExternals(const std::shared_ptr<CountingAssetAccessor>& pAccessor) {
  return TilesetExternals{
      pAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem{std::make_shared<SimpleTaskProcessor>()},
      std::make_shared<CreditSystem>()};
}

TilesetContentLoaderResult<TilesetJsonLoader>
loadTileset(TilesetExternals& externals, const std::string& url) {
  Future<TilesetContentLoaderResult<TilesetJsonLoader>> future =
      TilesetJsonLoader::createLoader(externals, url, {});
  externals.asyncSystem.dispatchMainThreadTasks();
  return future.wait();
}

TileLoadResult loadTile(
    TilesetExternals& externals,
    TilesetContentLoader& loader,
    Tile& tile) {
  TilesetContentOptions contentOptions;
  std::vector<IAssetAccessor::THeader> requestHeaders;
  TileLoadInput loadInput(
      tile,
      contentOptions,
      externals.asyncSystem,
      externals.pAssetAccessor,
      externals.pLogger,
      requestHeaders);
  loadInput.pSharedAssetSystem = externals.pSharedAssetSystem;
  Future<TileLoadResult> future = loader.loadTileContent(loadInput);
  externals.asyncSystem.dispatchMainThreadTasks();
  return future.wait();
}

Schema* schemaFromModel(Model& model) {
  ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  if (pMetadata == nullptr || pMetadata->schema == nullptr) {
    return nullptr;
  }
  return pMetadata->schema.get();
}

} // namespace

TEST_CASE("MAXAR schemas are shared across external tileset loaders") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  const std::string rootUrl = "https://maxar.test/tileset.json";
  const std::string schemaUrl = "https://maxar.test/ships_schema.json";
  const std::string externalA = "https://maxar.test/a/external.json";
  const std::string externalB = "https://maxar.test/b/external.json";
  const std::string geoJsonA = "https://maxar.test/a/content.geojson";
  const std::string geoJsonB = "https://maxar.test/b/content.geojson";

  // The schema file lives next to the root tileset. Each external tileset
  // repeats the relative URI "ships_schema.json", which would point at a
  // different file if it were resolved against that external tileset.
  const std::string rootJson =
      tilesetWithSchemaUri("ships_schema.json", std::string(R"(,
      "refine": "ADD",
      "children": [
        {
          "boundingVolume": {
            "region": [-1.0, -0.5, 1.0, 0.5, 0.0, 10.0]
          },
          "geometricError": 0,
          "content": { "uri": ")") + externalA + R"(" }
        },
        {
          "boundingVolume": {
            "region": [-1.0, -0.5, 1.0, 0.5, 0.0, 10.0]
          },
          "geometricError": 0,
          "content": { "uri": ")" + externalB + R"(" }
        }
      ])");
  const std::string externalJson =
      tilesetWithSchemaUri("ships_schema.json", std::string(R"(,
      "content": { "uri": ")") + geoJsonA + R"(" })");
  const std::string externalJsonB =
      tilesetWithSchemaUri("ships_schema.json", std::string(R"(,
      "content": { "uri": ")") + geoJsonB + R"(" })");

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requests{
      {rootUrl, makeRequest(rootUrl, 200, rootJson)},
      {schemaUrl, makeRequest(schemaUrl, 200, schemaJson)},
      {externalA, makeRequest(externalA, 200, externalJson)},
      {externalB, makeRequest(externalB, 200, externalJsonB)},
      {geoJsonA, makeRequest(geoJsonA, 200, geoJson)},
      {geoJsonB, makeRequest(geoJsonB, 200, geoJson)}};
  std::shared_ptr<CountingAssetAccessor> pAccessor =
      std::make_shared<CountingAssetAccessor>(std::move(requests));
  TilesetExternals externals = makeExternals(pAccessor);

  TilesetContentLoaderResult<TilesetJsonLoader> loaderResult =
      loadTileset(externals, rootUrl);
  REQUIRE(loaderResult.pLoader);
  REQUIRE(loaderResult.pRootTile);
  Schema* pRootSchema = loaderResult.pLoader->getExternalSchema().get();
  REQUIRE(pRootSchema != nullptr);
  CHECK(pRootSchema->classes.find("ships") != pRootSchema->classes.end());
  CHECK(pAccessor->requestCounts[schemaUrl] == 1);

  REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
  Tile& originalRoot = loaderResult.pRootTile->getChildren()[0];
  REQUIRE(originalRoot.getChildren().size() == 2);

  Schema* modelSchemas[2] = {nullptr, nullptr};
  Schema* loaderSchemas[2] = {nullptr, nullptr};
  for (int32_t i = 0; i < 2; ++i) {
    Tile& externalTile = originalRoot.getChildren()[size_t(i)];
    TileLoadResult externalResult =
        loadTile(externals, *loaderResult.pLoader, externalTile);
    REQUIRE(externalResult.state == TileLoadResultState::Success);
    REQUIRE(externalResult.tileInitializer);
    externalTile.getContent().setContentKind(
        std::make_unique<TileExternalContent>(
            std::get<TileExternalContent>(externalResult.contentKind)));
    externalResult.tileInitializer(externalTile);
    REQUIRE(externalTile.getChildren().size() == 1);

    Tile& geoJsonTile = externalTile.getChildren()[0];
    TilesetContentLoader* pExternalLoader = geoJsonTile.getLoader();
    REQUIRE(pExternalLoader != nullptr);
    REQUIRE(pExternalLoader != loaderResult.pLoader.get());
    TileLoadResult geoJsonResult =
        loadTile(externals, *pExternalLoader, geoJsonTile);
    REQUIRE(geoJsonResult.state == TileLoadResultState::Success);
    Model* pModel = std::get_if<Model>(&geoJsonResult.contentKind);
    REQUIRE(pModel != nullptr);
    modelSchemas[i] = schemaFromModel(*pModel);
    loaderSchemas[i] = pExternalLoader->getExternalSchema().get();
  }

  CHECK(pAccessor->requestCounts[schemaUrl] == 1);
  CHECK(modelSchemas[0] == pRootSchema);
  CHECK(modelSchemas[1] == pRootSchema);
  CHECK(loaderSchemas[0] == pRootSchema);
  CHECK(loaderSchemas[1] == pRootSchema);
}

TEST_CASE("Destroying one MAXAR schema loader keeps the schema for the other") {
  const std::string schemaUrl = "https://maxar-lifetime.test/schema.json";
  const std::string tilesetA = "https://maxar-lifetime.test/a/tileset.json";
  const std::string tilesetB = "https://maxar-lifetime.test/b/tileset.json";
  const std::string tilesetJson = tilesetWithSchemaUri(schemaUrl, "");

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requests{
      {schemaUrl, makeRequest(schemaUrl, 200, schemaJson)},
      {tilesetA, makeRequest(tilesetA, 200, tilesetJson)},
      {tilesetB, makeRequest(tilesetB, 200, tilesetJson)}};
  std::shared_ptr<CountingAssetAccessor> pAccessor =
      std::make_shared<CountingAssetAccessor>(std::move(requests));
  TilesetExternals externals = makeExternals(pAccessor);

  TilesetContentLoaderResult<TilesetJsonLoader> first =
      loadTileset(externals, tilesetA);
  TilesetContentLoaderResult<TilesetJsonLoader> second =
      loadTileset(externals, tilesetB);
  REQUIRE(first.pLoader);
  REQUIRE(second.pLoader);

  IntrusivePointer<Schema> pSchema = first.pLoader->getExternalSchema();
  REQUIRE(pSchema != nullptr);
  CHECK(second.pLoader->getExternalSchema().get() == pSchema.get());
  CHECK(pAccessor->requestCounts[schemaUrl] == 1);

  first.pLoader.reset();
  pSchema.reset();

  IntrusivePointer<Schema> pRemaining = second.pLoader->getExternalSchema();
  REQUIRE(pRemaining != nullptr);
  CHECK(pRemaining->id == "default");
  CHECK(pRemaining->classes.find("ships") != pRemaining->classes.end());
}

TEST_CASE("A failed MAXAR schema load stays empty and is not fetched twice") {
  const std::string schemaUrl = "https://maxar-missing.test/schema.json";
  const std::string tilesetA = "https://maxar-missing.test/tileset.json";
  const std::string tilesetB = "https://maxar-missing.test/other/tileset.json";
  const std::string geoJsonUrl = "https://maxar-missing.test/content.geojson";
  const std::string tilesetJson = tilesetWithSchemaUri(
      schemaUrl,
      std::string(R"(,
      "content": { "uri": ")") +
          geoJsonUrl + R"(" })");

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requests{
      {schemaUrl, makeRequest(schemaUrl, 404, "")},
      {tilesetA, makeRequest(tilesetA, 200, tilesetJson)},
      {tilesetB, makeRequest(tilesetB, 200, tilesetJson)},
      {geoJsonUrl, makeRequest(geoJsonUrl, 200, geoJson)}};
  std::shared_ptr<CountingAssetAccessor> pAccessor =
      std::make_shared<CountingAssetAccessor>(std::move(requests));
  TilesetExternals externals = makeExternals(pAccessor);

  TilesetContentLoaderResult<TilesetJsonLoader> first =
      loadTileset(externals, tilesetA);
  REQUIRE(first.pLoader);
  REQUIRE(first.pRootTile);
  CHECK(first.pLoader->getExternalSchema().get() == nullptr);
  CHECK(pAccessor->requestCounts[schemaUrl] == 1);

  REQUIRE(first.pRootTile->getChildren().size() == 1);
  Tile& contentTile = first.pRootTile->getChildren()[0];
  TileLoadResult contentResult =
      loadTile(externals, *first.pLoader, contentTile);
  CHECK(contentResult.state == TileLoadResultState::Success);
  Model* pModel = std::get_if<Model>(&contentResult.contentKind);
  REQUIRE(pModel != nullptr);
  CHECK(schemaFromModel(*pModel) == nullptr);

  TilesetContentLoaderResult<TilesetJsonLoader> second =
      loadTileset(externals, tilesetB);
  REQUIRE(second.pLoader);
  CHECK(second.pLoader->getExternalSchema().get() == nullptr);
  CHECK(pAccessor->requestCounts[schemaUrl] == 1);
}

TEST_CASE("Malformed MAXAR schema JSON does not crash and is cached") {
  const std::string schemaUrl = "https://maxar-bad-schema.test/schema.json";
  const std::string tilesetA = "https://maxar-bad-schema.test/tileset.json";
  const std::string tilesetB =
      "https://maxar-bad-schema.test/other/tileset.json";
  const std::string tilesetJson = tilesetWithSchemaUri(schemaUrl, "");

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requests{
      {schemaUrl, makeRequest(schemaUrl, 200, "{")},
      {tilesetA, makeRequest(tilesetA, 200, tilesetJson)},
      {tilesetB, makeRequest(tilesetB, 200, tilesetJson)}};
  std::shared_ptr<CountingAssetAccessor> pAccessor =
      std::make_shared<CountingAssetAccessor>(std::move(requests));
  TilesetExternals externals = makeExternals(pAccessor);

  TilesetContentLoaderResult<TilesetJsonLoader> first =
      loadTileset(externals, tilesetA);
  REQUIRE(first.pLoader);
  CHECK(first.pLoader->getExternalSchema().get() == nullptr);

  TilesetContentLoaderResult<TilesetJsonLoader> second =
      loadTileset(externals, tilesetB);
  REQUIRE(second.pLoader);
  CHECK(second.pLoader->getExternalSchema().get() == nullptr);
  CHECK(pAccessor->requestCounts[schemaUrl] == 1);
}
