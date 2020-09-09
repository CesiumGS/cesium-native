#pragma once

#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContent.h"
#include <glm/mat4x4.hpp>
#include <gsl/span>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API GltfContent : public TileContent {
    public:
        static std::string TYPE;

        GltfContent(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url);
        GltfContent(const Tile& tile, tinygltf::Model&& data, const std::string& url);

        const tinygltf::Model& gltf() const { return this->_gltf; }

        virtual const std::string& getType() const { return TYPE; }
        virtual void finalizeLoad(Tile& /*tile*/) {}

        virtual void createRasterOverlayTextureCoordinates(
            uint32_t textureCoordinateID,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::Rectangle& rectangle
        ) override;

        typedef void ForEachPrimitiveInSceneCallback(
            tinygltf::Model& gltf,
            tinygltf::Node& node,
            tinygltf::Mesh& mesh,
            tinygltf::Primitive& primitive,
            const glm::dmat4& transform
        );
        typedef void ForEachPrimitiveInSceneConstCallback(
            const tinygltf::Model& gltf,
            const tinygltf::Node& node,
            const tinygltf::Mesh& mesh,
            const tinygltf::Primitive& primitive,
            const glm::dmat4& transform
        );

        void forEachPrimitiveInScene(int sceneID, std::function<ForEachPrimitiveInSceneCallback>&& callback);
        void forEachPrimitiveInScene(int sceneID, std::function<ForEachPrimitiveInSceneConstCallback>&& callback) const;

    private:
        tinygltf::Model _gltf;
    };

}
