#include <Cesium3DTilesContent/CmptToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace Cesium3DTilesContent {
namespace {
struct CmptHeader {
  char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t tilesLength;
};

struct InnerHeader {
  char magic[4];
  uint32_t version;
  uint32_t byteLength;
};

static_assert(sizeof(CmptHeader) == 16);
static_assert(sizeof(InnerHeader) == 12);
} // namespace

CesiumAsync::Future<GltfConverterResult> CmptToGltfConverter::convert(
    const std::span<const std::byte>& cmptBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  GltfConverterResult result;
  if (cmptBinary.size() < sizeof(CmptHeader)) {
    result.errors.emplaceWarning("Composite tile must be at least 16 bytes.");
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  const CmptHeader* pHeader =
      reinterpret_cast<const CmptHeader*>(cmptBinary.data());
  if (std::string(pHeader->magic, 4) != "cmpt") {
    result.errors.emplaceWarning(
        "Composite tile does not have the expected magic vaue 'cmpt'.");
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  if (pHeader->version != 1) {
    result.errors.emplaceWarning(fmt::format(
        "Unsupported composite tile version {}.",
        pHeader->version));
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  if (pHeader->byteLength > cmptBinary.size()) {
    result.errors.emplaceWarning(fmt::format(
        "Composite tile byteLength is {} but only {} bytes are available.",
        pHeader->byteLength,
        cmptBinary.size()));
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  std::vector<CesiumAsync::Future<GltfConverterResult>> innerTiles;
  uint32_t pos = sizeof(CmptHeader);

  for (uint32_t i = 0; i < pHeader->tilesLength && pos < pHeader->byteLength;
       ++i) {
    if (pos + sizeof(InnerHeader) > pHeader->byteLength) {
      result.errors.emplaceWarning(
          "Composite tile ends before all embedded tiles could be read.");
      break;
    }
    const InnerHeader* pInner =
        reinterpret_cast<const InnerHeader*>(cmptBinary.data() + pos);
    if (pos + pInner->byteLength > pHeader->byteLength) {
      result.errors.emplaceWarning(
          "Composite tile ends before all embedded tiles could be read.");
      break;
    }

    const std::span<const std::byte> innerData(
        cmptBinary.data() + pos,
        pInner->byteLength);

    pos += pInner->byteLength;

    innerTiles.emplace_back(
        GltfConverters::convert(innerData, options, assetFetcher));
  }

  uint32_t tilesLength = pHeader->tilesLength;
  if (innerTiles.empty()) {
    if (tilesLength > 0) {
      result.errors.emplaceWarning(
          "Composite tile does not contain any loadable inner "
          "tiles.");
    }
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  return assetFetcher.asyncSystem.all(std::move(innerTiles))
      .thenImmediately([](std::vector<GltfConverterResult>&& innerResults) {
        if (innerResults.size() == 1) {
          return innerResults[0];
        }
        GltfConverterResult cmptResult;
        for (auto& innerTile : innerResults) {
          if (innerTile.model) {
            if (cmptResult.model) {
              cmptResult.model->merge(std::move(*innerTile.model));
            } else {
              cmptResult.model = std::move(innerTile.model);
            }
          }
          cmptResult.errors.merge(innerTile.errors);
        }
        return cmptResult;
      });
}
} // namespace Cesium3DTilesContent
