// Copyright CesiumGS, Inc. and Contributors

#include "SkirtMeshMetadata.h"

using namespace CesiumGltf;

namespace Cesium3DTiles {
    std::optional<SkirtMeshMetadata> SkirtMeshMetadata::parseFromGltfExtras(const JsonValue::Object& extras) {
        auto skirtIt = extras.find("skirtMeshMetadata");
        if (skirtIt == extras.end()) {
            return std::nullopt;
        }

        SkirtMeshMetadata skirtMeshMetadata;

        const JsonValue& gltfSkirtMeshMetadata = skirtIt->second;
        const JsonValue::Array* pNoSkirtRange = gltfSkirtMeshMetadata.getValueForKey<JsonValue::Array>("noSkirtRange");
        if (!pNoSkirtRange || pNoSkirtRange->size() != 2) {
            return std::nullopt;
        }

        if (!(*pNoSkirtRange)[0].isNumber() || !(*pNoSkirtRange)[1].isNumber()) {
            return std::nullopt;
        }

        double noSkirtIndicesBegin = (*pNoSkirtRange)[0].getNumber(-1.0);
        double noSkirtIndicesCount = (*pNoSkirtRange)[1].getNumber(-1.0);

        if (noSkirtIndicesBegin < 0.0 || noSkirtIndicesCount < 0.0) {
            return std::nullopt;
        }

        skirtMeshMetadata.noSkirtIndicesBegin = static_cast<uint32_t>(noSkirtIndicesBegin);
        skirtMeshMetadata.noSkirtIndicesCount = static_cast<uint32_t>(noSkirtIndicesCount);

        const JsonValue::Array* pMeshCenter = gltfSkirtMeshMetadata.getValueForKey<JsonValue::Array>("meshCenter");
        if (!pMeshCenter || pMeshCenter->size() != 3) {
            return std::nullopt;
        }

        if (!(*pMeshCenter)[0].isNumber() || !(*pMeshCenter)[1].isNumber() || !(*pMeshCenter)[2].isNumber()) {
            return std::nullopt;
        }

        skirtMeshMetadata.meshCenter = glm::dvec3(
            (*pMeshCenter)[0].getNumber(0.0),
            (*pMeshCenter)[1].getNumber(0.0),
            (*pMeshCenter)[2].getNumber(0.0)
        );

        const double* pWestHeight = gltfSkirtMeshMetadata.getValueForKey<JsonValue::Number>("skirtWestHeight");
        const double* pSouthHeight = gltfSkirtMeshMetadata.getValueForKey<JsonValue::Number>("skirtSouthHeight");
        const double* pEastHeight = gltfSkirtMeshMetadata.getValueForKey<JsonValue::Number>("skirtEastHeight");
        const double* pNorthHeight = gltfSkirtMeshMetadata.getValueForKey<JsonValue::Number>("skirtNorthHeight");

        if (!pWestHeight || !pSouthHeight || !pEastHeight || !pNorthHeight) {
            return std::nullopt;
        }

        skirtMeshMetadata.skirtWestHeight = *pWestHeight;
        skirtMeshMetadata.skirtSouthHeight = *pSouthHeight;
        skirtMeshMetadata.skirtEastHeight = *pEastHeight;
        skirtMeshMetadata.skirtNorthHeight = *pNorthHeight;

        return skirtMeshMetadata;
    }

    CesiumGltf::JsonValue::Object SkirtMeshMetadata::createGltfExtras(const SkirtMeshMetadata& skirtMeshMetadata) {
        return {
            { "skirtMeshMetadata", JsonValue::Object {
                { "noSkirtRange", JsonValue::Array { skirtMeshMetadata.noSkirtIndicesBegin, skirtMeshMetadata.noSkirtIndicesCount } },
                { "meshCenter", JsonValue::Array { skirtMeshMetadata.meshCenter.x, skirtMeshMetadata.meshCenter.y, skirtMeshMetadata.meshCenter.z } },
                { "skirtWestHeight", skirtMeshMetadata.skirtWestHeight },
                { "skirtSouthHeight", skirtMeshMetadata.skirtSouthHeight },
                { "skirtEastHeight", skirtMeshMetadata.skirtEastHeight },
                { "skirtNorthHeight", skirtMeshMetadata.skirtNorthHeight }
            } }
        };
    }
}