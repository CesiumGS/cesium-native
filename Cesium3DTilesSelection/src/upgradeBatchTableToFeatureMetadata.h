#pragma once

#include <gsl/span>
#include <rapidjson/fwd.h>
#include <spdlog/fwd.h>

#include <memory>

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTilesSelection {

/**
 * @brief Parses the provided B3DM batch table and adds an equivalent
 * EXT_feature_metadata extension to the provided glTF.
 *
 * @param pLogger
 * @param gltf
 * @param featureTable
 * @param batchTableJson
 * @param batchTableBinaryData
 */
void upgradeBatchTableToFeatureMetadata(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const rapidjson::Document& featureTable,
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData);

} // namespace Cesium3DTilesSelection
