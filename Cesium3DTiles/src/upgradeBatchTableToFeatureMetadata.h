#pragma once

#include <gsl/span>
#include <memory>
#include <rapidjson/fwd.h>
#include <spdlog/fwd.h>

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTiles {

/**
 * @brief Parses the provided B3DM batch table and adds an equivalent
 * EXT_feature_metadata extension to the provided glTF.
 *
 * @param pLogger
 * @param gltf
 * @param featureTable
 * @param batchTableJsonData
 * @param batchTableBinaryData
 */
void upgradeBatchTableToFeatureMetadata(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const rapidjson::Document& featureTable,
    const gsl::span<const std::byte>& batchTableJsonData,
    const gsl::span<const std::byte>& batchTableBinaryData);

} // namespace Cesium3DTiles
