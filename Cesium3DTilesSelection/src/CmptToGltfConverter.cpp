#include "CmptToGltfConverter.h"

#include <Cesium3DTilesSelection/GltfConverters.h>

#include <spdlog/fmt/fmt.h>

namespace Cesium3DTilesSelection {
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

GltfConverterResult CmptToGltfConverter::convert(
    const gsl::span<const std::byte>& cmptBinary,
    const CesiumGltfReader::GltfReaderOptions& options) {
  GltfConverterResult result;
  if (cmptBinary.size() < sizeof(CmptHeader)) {
    result.errors.emplaceWarning("Composite tile must be at least 16 bytes.");
    return result;
  }

  const CmptHeader* pHeader =
      reinterpret_cast<const CmptHeader*>(cmptBinary.data());
  if (std::string(pHeader->magic, 4) != "cmpt") {
    result.errors.emplaceWarning(
        "Composite tile does not have the expected magic vaue 'cmpt'.");
    return result;
  }

  if (pHeader->version != 1) {
    result.errors.emplaceWarning(fmt::format(
        "Unsupported composite tile version {}.",
        pHeader->version));
    return result;
  }

  if (pHeader->byteLength > cmptBinary.size()) {
    result.errors.emplaceWarning(fmt::format(
        "Composite tile byteLength is {} but only {} bytes are available.",
        pHeader->byteLength,
        cmptBinary.size()));
    return result;
  }

  std::vector<GltfConverterResult> innerTiles;
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

    const gsl::span<const std::byte> innerData(
        cmptBinary.data() + pos,
        pInner->byteLength);

    pos += pInner->byteLength;

    innerTiles.emplace_back(GltfConverters::convert(innerData, options));
  }

  uint32_t tilesLength = pHeader->tilesLength;
  if (innerTiles.empty()) {
    if (tilesLength > 0) {
      result.errors.emplaceWarning(
          "Composite tile does not contain any loadable inner "
          "tiles.");
    }

    return result;
  }

  if (innerTiles.size() == 1) {
    return std::move(innerTiles[0]);
  }

  for (size_t i = 0; i < innerTiles.size(); ++i) {
    if (innerTiles[i].model) {
      if (result.model) {
        result.model->merge(std::move(*innerTiles[i].model));
      } else {
        result.model = std::move(innerTiles[i].model);
      }
    }

    result.errors.merge(innerTiles[i].errors);
  }

  return result;
}
} // namespace Cesium3DTilesSelection
