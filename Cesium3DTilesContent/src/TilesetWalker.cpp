#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesContent/TilesetWalker.h>
#include <Cesium3DTilesReader/TilesetReader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <nonstd/expected.hpp>

using namespace Cesium3DTiles;
using namespace Cesium3DTilesReader;
using namespace CesiumAsync;
using namespace CesiumGltfReader;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace Cesium3DTilesContent {

TilesetWalker::TilesetWalker(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor)
    : _asyncSystem(asyncSystem), _pAssetAccessor(pAssetAccessor) {}

Future<void> TilesetWalker::walkDepthFirst(
    const std::shared_ptr<TilesetVisitor>& pVisitor,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers) {
  return this->_pAssetAccessor->get(this->_asyncSystem, url, headers)
      .thenInWorkerThread(
          [url](std::shared_ptr<IAssetRequest>&& pRequest)
              -> nonstd::expected<Tileset, ErrorList> {
            const IAssetResponse* pResponse = pRequest->response();
            if (pResponse == nullptr) {
              ErrorList error;
              error.errors.emplace_back(fmt::format(
                  "Did not receive a response from URL `{}`.",
                  url));
              return nonstd::make_unexpected(std::move(error));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode < 200 || statusCode >= 300) {
              ErrorList error;
              error.errors.emplace_back(fmt::format(
                  "Request for URL `{}` failed with status code {}.",
                  url,
                  statusCode));
              return nonstd::make_unexpected(std::move(error));
            }

            TilesetReader reader;
            ReadJsonResult<Tileset> readResult =
                reader.readFromJson(pResponse->data());
            if (!readResult.value) {
              // TODO: see if this is a layer.json instead.
              ErrorList error;
              error.errors = std::move(readResult.errors);
              error.warnings = std::move(readResult.warnings);
              return nonstd::make_unexpected(std::move(error));
            }

            return *readResult.value;
          })
      .thenInMainThread([this, pVisitor, url, headers](
                            nonstd::expected<Tileset, ErrorList>&& loadResult) {
        if (loadResult) {
          this->walkDepthFirst(pVisitor, url, headers);
        }
      });
}

Future<void> TilesetWalker::walkDepthFirst(
    const std::shared_ptr<TilesetVisitor>& pVisitor,
    Tileset& tileset,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
  TilesetWalkerControl control;

  Tile& tile = tileset.root;
  this->walkRecursively(pVisitor, control, tile, url, headers);

  return this->_asyncSystem.createResolvedFuture();
}

void TilesetWalker::walkRecursively(
    const std::shared_ptr<TilesetVisitor>& pVisitor,
    TilesetWalkerControl& control,
    Tile& tile,
    const std::string& baseUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
  control.reset();
  pVisitor->visitTile(control, tile);

  if (control.shouldVisitContent()) {
    this->walkContent(pVisitor, control, tile, baseUrl, headers);
  }

  if (control.shouldVisitChildren()) {
    pVisitor->visitChildrenBegin(control, tile);

    for (Tile& child : tile.children) {
      this->walkRecursively(pVisitor, control, child, baseUrl, headers);
    }

    pVisitor->visitChildrenEnd(control, tile);
  }
}

void TilesetWalker::walkContent(
    const std::shared_ptr<TilesetVisitor>& pVisitor,
    TilesetWalkerControl& control,
    Cesium3DTiles::Tile& tile,
    const std::string& baseUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
  if (!tile.content || tile.content->uri.empty()) {
    pVisitor->visitNoContent(control, tile);
    return;
  }

  std::string url = Uri::resolve(baseUrl, tile.content->uri, true);
  std::shared_ptr<IAssetRequest> pRequest =
      this->_pAssetAccessor->get(this->_asyncSystem, url, headers).wait();

  const IAssetResponse* pResponse = pRequest->response();
  if (pResponse == nullptr) {
    pVisitor->onError(&tile, pRequest.get(), "Did not receive a response.");
    return;
  }

  uint16_t statusCode = pResponse->statusCode();
  if (statusCode < 200 || statusCode >= 300) {
    pVisitor->onError(
        &tile,
        pRequest.get(),
        fmt::format("Request failed with status code {}.", statusCode));
    return;
  }

  gsl::span<const std::byte> data = pResponse->data();

  GltfReaderOptions options;
  GltfConverterResult result = GltfConverters::convert(url, data, options);
  if (!result.model || result.errors) {
    // Failed to convert to glTF. This is either an external tileset, or an
    // unsupported payload time.
    // TODO: check if its an external tileset and load it.
    pVisitor->visitUnknownContent(control, tile, *pRequest);
    return;
  }

  pVisitor->visitModelContent(control, tile, *pRequest, *result.model);
}

} // namespace Cesium3DTilesContent
