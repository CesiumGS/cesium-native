#include "SkirtMeshMetadata.h"

namespace Cesium3DTiles {
    std::optional<SkirtMeshMetadata> SkirtMeshMetadata::parseFromGltfExtras(const tinygltf::Value& extras) {
        if (extras.IsObject() && extras.Has("skirtMeshMetadata")) {
            SkirtMeshMetadata skirtMeshMetadata;

            tinygltf::Value skirts = extras.Get("skirtMeshMetadata");
            tinygltf::Value noSkirtRange = skirts.Get("noSkirtRange");
			skirtMeshMetadata.noSkirtIndicesBegin = static_cast<uint32_t>(noSkirtRange.Get(0).GetNumberAsInt());
			skirtMeshMetadata.noSkirtIndicesCount = static_cast<uint32_t>(noSkirtRange.Get(1).GetNumberAsInt());

            tinygltf::Value meshCenter = skirts.Get("meshCenter");
            skirtMeshMetadata.meshCenter.x = meshCenter.Get(0).GetNumberAsDouble();
            skirtMeshMetadata.meshCenter.y = meshCenter.Get(1).GetNumberAsDouble();
            skirtMeshMetadata.meshCenter.z = meshCenter.Get(2).GetNumberAsDouble();

            tinygltf::Value skirtWestHeight = skirts.Get("skirtWestHeight");
            skirtMeshMetadata.skirtWestHeight = skirtWestHeight.GetNumberAsDouble();

            tinygltf::Value skirtSouthHeight = skirts.Get("skirtSouthHeight");
            skirtMeshMetadata.skirtSouthHeight = skirtSouthHeight.GetNumberAsDouble();

            tinygltf::Value skirtEastHeight = skirts.Get("skirtEastHeight");
            skirtMeshMetadata.skirtEastHeight = skirtEastHeight.GetNumberAsDouble();

            tinygltf::Value skirtNorthHeight = skirts.Get("skirtNorthHeight");
            skirtMeshMetadata.skirtNorthHeight = skirtNorthHeight.GetNumberAsDouble();

            return skirtMeshMetadata;
        }

        return std::nullopt;
    }

    tinygltf::Value SkirtMeshMetadata::createGltfExtras(const SkirtMeshMetadata& skirtMeshMetadata) {
		tinygltf::Value::Object extras;
		extras.insert({ "noSkirtRange", tinygltf::Value(tinygltf::Value::Array({
			tinygltf::Value(static_cast<int>(skirtMeshMetadata.noSkirtIndicesBegin)), tinygltf::Value(static_cast<int>(skirtMeshMetadata.noSkirtIndicesCount))})) });

		extras.insert({ "meshCenter", tinygltf::Value(tinygltf::Value::Array({
			tinygltf::Value(skirtMeshMetadata.meshCenter.x), tinygltf::Value(skirtMeshMetadata.meshCenter.y), tinygltf::Value(skirtMeshMetadata.meshCenter.z)})) });

		extras.insert({ "skirtWestHeight", tinygltf::Value(skirtMeshMetadata.skirtWestHeight) });
		extras.insert({ "skirtSouthHeight", tinygltf::Value(skirtMeshMetadata.skirtSouthHeight) });
		extras.insert({ "skirtEastHeight", tinygltf::Value(skirtMeshMetadata.skirtEastHeight) });
		extras.insert({ "skirtNorthHeight", tinygltf::Value(skirtMeshMetadata.skirtNorthHeight) });

		return tinygltf::Value(
			tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(extras)} });
    }
}