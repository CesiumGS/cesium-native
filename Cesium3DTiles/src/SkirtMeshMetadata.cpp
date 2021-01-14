#include "SkirtMeshMetadata.h"

namespace Cesium3DTiles {
    std::optional<SkirtMeshMetadata> SkirtMeshMetadata::parseFromGltfExtras(const tinygltf::Value& extras) {
        if (extras.IsObject() && extras.Has("skirtMeshMetadata")) {
            SkirtMeshMetadata skirtMeshMetadata;

            tinygltf::Value gltfSkirtMeshMetadata = extras.Get("skirtMeshMetadata");
            if (!gltfSkirtMeshMetadata.IsObject()) {
                return std::nullopt;
            }

            if (gltfSkirtMeshMetadata.Has("noSkirtRange")) {
                tinygltf::Value noSkirtRange = gltfSkirtMeshMetadata.Get("noSkirtRange");
                if (noSkirtRange.IsArray() && noSkirtRange.ArrayLen() == 2) {
                    tinygltf::Value noSkirtIndicesBegin = noSkirtRange.Get(0);
                    tinygltf::Value noSkirtIndicesCount = noSkirtRange.Get(1);
                    if (noSkirtIndicesBegin.IsInt() && noSkirtIndicesCount.IsInt()) {
                        skirtMeshMetadata.noSkirtIndicesBegin = static_cast<uint32_t>(noSkirtRange.Get(0).GetNumberAsInt());
                        skirtMeshMetadata.noSkirtIndicesCount = static_cast<uint32_t>(noSkirtRange.Get(1).GetNumberAsInt());
                    }
                    else {
                        return std::nullopt;
                    }
                }
                else {
                    return std::nullopt;
                }
            }
            else {
                return std::nullopt;
            }

            if (gltfSkirtMeshMetadata.Has("meshCenter")) {
                tinygltf::Value meshCenter = gltfSkirtMeshMetadata.Get("meshCenter");
                if (meshCenter.IsArray() && meshCenter.ArrayLen() == 3) {
                    tinygltf::Value x = meshCenter.Get(0);
                    tinygltf::Value y = meshCenter.Get(1);
                    tinygltf::Value z = meshCenter.Get(2);
                    if (x.IsNumber() && y.IsNumber() && z.IsNumber()) {
                        skirtMeshMetadata.meshCenter.x = x.GetNumberAsDouble();
                        skirtMeshMetadata.meshCenter.y = y.GetNumberAsDouble();
                        skirtMeshMetadata.meshCenter.z = z.GetNumberAsDouble();
                    }
                    else {
                        return std::nullopt;
                    }
                }
                else {
                    return std::nullopt;
                }
            }
            else {
                return std::nullopt;
            }

            if (gltfSkirtMeshMetadata.Has("skirtWestHeight")) {
                tinygltf::Value skirtWestHeight = gltfSkirtMeshMetadata.Get("skirtWestHeight");
                if (skirtWestHeight.IsNumber()) {
                    skirtMeshMetadata.skirtWestHeight = skirtWestHeight.GetNumberAsDouble();
                }
                else {
                    return std::nullopt;
                }
            }
            else {
                return std::nullopt;
            }

            if (gltfSkirtMeshMetadata.Has("skirtSouthHeight")) {
                tinygltf::Value skirtSouthHeight = gltfSkirtMeshMetadata.Get("skirtSouthHeight");
                if (skirtSouthHeight.IsNumber()) {
                    skirtMeshMetadata.skirtSouthHeight = skirtSouthHeight.GetNumberAsDouble();
                }
                else {
                    return std::nullopt;
                }
            }
            else {
                return std::nullopt;
            }

            if (gltfSkirtMeshMetadata.Has("skirtEastHeight")) {
                tinygltf::Value skirtEastHeight = gltfSkirtMeshMetadata.Get("skirtEastHeight");
                if (skirtEastHeight.IsNumber()) {
                    skirtMeshMetadata.skirtEastHeight = skirtEastHeight.GetNumberAsDouble();
                }
                else {
                    return std::nullopt;
                }
            }
            else {
                return std::nullopt;
            }

            if (gltfSkirtMeshMetadata.Has("skirtNorthHeight")) {
                tinygltf::Value skirtNorthHeight = gltfSkirtMeshMetadata.Get("skirtNorthHeight");
                if (skirtNorthHeight.IsNumber()) {
                    skirtMeshMetadata.skirtNorthHeight = skirtNorthHeight.GetNumberAsDouble();
                }
                else {
                    return std::nullopt;
                }
            }
            else {
                return std::nullopt;
            }

            return skirtMeshMetadata;
        }

        return std::nullopt;
    }

    tinygltf::Value SkirtMeshMetadata::createGltfExtras(const SkirtMeshMetadata& skirtMeshMetadata) {
        tinygltf::Value::Object gltfSkirtMeshMetadata;
        gltfSkirtMeshMetadata.insert({ "noSkirtRange", tinygltf::Value(tinygltf::Value::Array({
            tinygltf::Value(static_cast<int>(skirtMeshMetadata.noSkirtIndicesBegin)), tinygltf::Value(static_cast<int>(skirtMeshMetadata.noSkirtIndicesCount))})) });

        gltfSkirtMeshMetadata.insert({ "meshCenter", tinygltf::Value(tinygltf::Value::Array({
            tinygltf::Value(skirtMeshMetadata.meshCenter.x), tinygltf::Value(skirtMeshMetadata.meshCenter.y), tinygltf::Value(skirtMeshMetadata.meshCenter.z)})) });

        gltfSkirtMeshMetadata.insert({ "skirtWestHeight", tinygltf::Value(skirtMeshMetadata.skirtWestHeight) });
        gltfSkirtMeshMetadata.insert({ "skirtSouthHeight", tinygltf::Value(skirtMeshMetadata.skirtSouthHeight) });
        gltfSkirtMeshMetadata.insert({ "skirtEastHeight", tinygltf::Value(skirtMeshMetadata.skirtEastHeight) });
        gltfSkirtMeshMetadata.insert({ "skirtNorthHeight", tinygltf::Value(skirtMeshMetadata.skirtNorthHeight) });

        return tinygltf::Value(
            tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });
    }
}