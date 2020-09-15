#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/GltfAccessor.h"
#include "CesiumUtility/Math.h"
#include <stdexcept>

namespace Cesium3DTiles {

	std::string GltfContent::TYPE = "gltf";

	GltfContent::GltfContent(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& /*url*/) :
		TileContent(tile)
	{
		tinygltf::TinyGLTF loader;
		std::string errors;
		std::string warnings;

		/*bool loadSucceeded =*/ loader.LoadBinaryFromMemory(&this->_gltf, &errors, &warnings, data.data(), static_cast<unsigned int>(data.size()));
		//if (!loadSucceeded) {
		//	throw std::runtime_error("Failed to load glTF model from B3DM.");
		//}
	}

	GltfContent::GltfContent(const Tile& tile, tinygltf::Model&& data, const std::string& /*url*/) :
		TileContent(tile),
		_gltf(std::move(data))
	{
	}

	static int generateOverlayTextureCoordinates(
		tinygltf::Model& gltf,
		int positionAccessorIndex,
		const glm::dmat4x4& transform,
		const CesiumGeospatial::Projection& projection,
		const CesiumGeometry::Rectangle& rectangle
	) {
        int uvBufferId = static_cast<int>(gltf.buffers.size());
        gltf.buffers.emplace_back();

        int uvBufferViewId = static_cast<int>(gltf.bufferViews.size());
        gltf.bufferViews.emplace_back();

        int uvAccessorId = static_cast<int>(gltf.accessors.size());
        gltf.accessors.emplace_back();

		GltfAccessor<glm::vec3> positionAccessor(gltf, positionAccessorIndex);

        tinygltf::Buffer& uvBuffer = gltf.buffers[uvBufferId];
        uvBuffer.data.resize(positionAccessor.size() * 2 * sizeof(float));

        tinygltf::BufferView& uvBufferView = gltf.bufferViews[uvBufferViewId];
        uvBufferView.buffer = uvBufferId;
        uvBufferView.byteOffset = 0;
        uvBufferView.byteStride = 2 * sizeof(float);
        uvBufferView.byteLength = uvBuffer.data.size();
        uvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

        tinygltf::Accessor& uvAccessor = gltf.accessors[uvAccessorId];
        uvAccessor.bufferView = uvBufferViewId;
        uvAccessor.byteOffset = 0;
        uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        uvAccessor.count = positionAccessor.size();
        uvAccessor.type = TINYGLTF_TYPE_VEC2;

		GltfWriter<glm::vec2> uvWriter(gltf, uvAccessorId);

		double width = rectangle.computeWidth();
		double height = rectangle.computeHeight();

		for (size_t i = 0; i < positionAccessor.size(); ++i) {
			// Get the ECEF position
			glm::vec3 position = positionAccessor[i];
			glm::dvec3 positionEcef = transform * glm::dvec4(position, 1.0);
			
			// Convert it to cartographic
			std::optional<CesiumGeospatial::Cartographic> cartographic = CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(positionEcef);
			if (!cartographic) {
				uvWriter[i] = glm::dvec2(0.0, 0.0);
				continue;
			}

			// Project it with the raster overlay's projection
			glm::dvec3 projectedPosition = projectPosition(projection, cartographic.value());

			// Scale to (0.0, 0.0) at the (minimumX, minimumY) corner, and (1.0, 1.0) at the (maximumX, maximumY) corner.
			// The coordinates should stay inside these bounds if the input rectangle actually bounds the vertices,
			// but we'll clamp to be safe.
			glm::vec2 uv(
				CesiumUtility::Math::clamp((projectedPosition.x - rectangle.minimumX) / width, 0.0, 1.0),
				CesiumUtility::Math::clamp((projectedPosition.y - rectangle.minimumY) / height, 0.0, 1.0)
			);

			uvWriter[i] = uv;
		}

		return uvAccessorId;
	}

	void GltfContent::createRasterOverlayTextureCoordinates(
		uint32_t textureCoordinateID,
		const CesiumGeospatial::Projection& projection,
		const CesiumGeometry::Rectangle& rectangle
	) {
		std::vector<int> positionAccessorsToTextureCoordinateAccessor;
		positionAccessorsToTextureCoordinateAccessor.resize(this->_gltf.accessors.size(), 0);

		std::string attributeName = "_CESIUMOVERLAY_" + std::to_string(textureCoordinateID);

		this->forEachPrimitiveInScene(-1, [&positionAccessorsToTextureCoordinateAccessor, &attributeName, &projection, &rectangle](
            tinygltf::Model& gltf,
            tinygltf::Node& /*node*/,
            tinygltf::Mesh& /*mesh*/,
            tinygltf::Primitive& primitive,
            const glm::dmat4& transform
		) {
			auto positionIt = primitive.attributes.find("POSITION");
			if (positionIt == primitive.attributes.end()) {
				return;
			}

			int positionAccessorIndex = positionIt->second;
			if (positionAccessorIndex < 0 || positionAccessorIndex >= static_cast<int>(gltf.accessors.size())) {
				return;
			}

			int textureCoordinateAccessorIndex = positionAccessorsToTextureCoordinateAccessor[positionAccessorIndex];
			if (textureCoordinateAccessorIndex > 0) {
				primitive.attributes[attributeName] = textureCoordinateAccessorIndex;
				return;
			}

			// TODO remove this check
			if (primitive.attributes.find(attributeName) != primitive.attributes.end()) {
				return;
			}

			// Generate new texture coordinates
			int nextTextureCoordinateAccessorIndex = generateOverlayTextureCoordinates(gltf, positionAccessorIndex, transform, projection, rectangle);
			primitive.attributes[attributeName] = nextTextureCoordinateAccessorIndex;
			positionAccessorsToTextureCoordinateAccessor[positionAccessorIndex] = nextTextureCoordinateAccessorIndex;
		});
	}

	void GltfContent::forEachPrimitiveInScene(int sceneID, std::function<ForEachPrimitiveInSceneCallback>&& callback) {
		return const_cast<const GltfContent*>(this)->forEachPrimitiveInScene(sceneID, [&callback](
            const tinygltf::Model& gltf,
			const tinygltf::Node& node,
            const tinygltf::Mesh& mesh,
            const tinygltf::Primitive& primitive,
            const glm::dmat4& transform
		) {
			callback(
				const_cast<tinygltf::Model&>(gltf),
				const_cast<tinygltf::Node&>(node),
				const_cast<tinygltf::Mesh&>(mesh),
				const_cast<tinygltf::Primitive&>(primitive),
				transform
			);
		});
	}

	// Initialize with a static function instead of inline to avoid an
	// internal compiler error in MSVC v14.27.29110.
	static glm::dmat4 createGltfAxesToCesiumAxes() {
		// https://github.com/CesiumGS/3d-tiles/tree/master/specification#gltf-transforms
		return glm::dmat4(
			glm::dvec4(1.0,  0.0, 0.0, 0.0),
			glm::dvec4(0.0,  0.0, 1.0, 0.0),
			glm::dvec4(0.0, -1.0, 0.0, 0.0),
			glm::dvec4(0.0,  0.0, 0.0, 1.0)
		);
	}

	glm::dmat4 gltfAxesToCesiumAxes = createGltfAxesToCesiumAxes();

	static void forEachPrimitiveInMeshObject(
		const glm::dmat4x4& transform,
		const tinygltf::Model& model,
		const tinygltf::Node& node,
		const tinygltf::Mesh& mesh,
		std::function<GltfContent::ForEachPrimitiveInSceneConstCallback>& callback
	) {
		for (const tinygltf::Primitive& primitive : mesh.primitives) {
			callback(model, node, mesh, primitive, transform);
		}
	}

	static void forEachPrimitiveInNodeObject(const glm::dmat4x4& transform, const tinygltf::Model& model, const tinygltf::Node& node, std::function<GltfContent::ForEachPrimitiveInSceneConstCallback>& callback) {
		glm::dmat4x4 nodeTransform = transform;

		if (node.matrix.size() > 0) {
			const std::vector<double>& matrix = node.matrix;

			glm::dmat4x4 nodeTransformGltf(
				glm::dvec4(matrix[0], matrix[1], matrix[2], matrix[3]),
				glm::dvec4(matrix[4], matrix[5], matrix[6], matrix[7]),
				glm::dvec4(matrix[8], matrix[9], matrix[10], matrix[11]),
				glm::dvec4(matrix[12], matrix[13], matrix[14], matrix[15])
			);

			nodeTransform = nodeTransform * nodeTransformGltf;
		}
		else if (node.translation.size() > 0 || node.rotation.size() > 0 || node.scale.size() > 0) {
			// TODO: handle this type of transformation
		}

		int meshId = node.mesh;
		if (meshId >= 0 && meshId < static_cast<int>(model.meshes.size())) {
			const tinygltf::Mesh& mesh = model.meshes[meshId];
			forEachPrimitiveInMeshObject(nodeTransform, model, node, mesh, callback);
		}

		for (int childNodeId : node.children) {
			if (childNodeId >= 0 && childNodeId < static_cast<int>(model.nodes.size())) {
				forEachPrimitiveInNodeObject(nodeTransform, model, model.nodes[childNodeId], callback);
			}
		}
	}

	static void forEachPrimitiveInSceneObject(const glm::dmat4x4& transform, const tinygltf::Model& model, const tinygltf::Scene& scene, std::function<GltfContent::ForEachPrimitiveInSceneConstCallback>& callback) {
		for (int nodeID : scene.nodes) {
			if (nodeID >= 0 && nodeID < static_cast<int>(model.nodes.size())) {
				forEachPrimitiveInNodeObject(transform, model, model.nodes[nodeID], callback);
			}
		}
	}

	void GltfContent::forEachPrimitiveInScene(int sceneID, std::function<ForEachPrimitiveInSceneConstCallback>&& callback) const {
		glm::dmat4x4 rootTransform = gltfAxesToCesiumAxes;

		const tinygltf::Model& model = this->_gltf;
		if (sceneID >= 0) {
			// Use the user-specified scene if it exists.
			if (sceneID < static_cast<int>(model.scenes.size())) {
				forEachPrimitiveInSceneObject(rootTransform, model, model.scenes[sceneID], callback);
			}
		} else if (model.defaultScene >= 0 && model.defaultScene < static_cast<int>(model.scenes.size())) {
			// Use the default scene
			forEachPrimitiveInSceneObject(rootTransform, model, model.scenes[model.defaultScene], callback);
		} else if (model.scenes.size() > 0) {
			// There's no default, so use the first scene
			const tinygltf::Scene& defaultScene = model.scenes[0];
			forEachPrimitiveInSceneObject(rootTransform, model, defaultScene, callback);
		} else if (model.nodes.size() > 0) {
			// No scenes at all, use the first node as the root node.
			forEachPrimitiveInNodeObject(rootTransform, model, model.nodes[0], callback);
		} else if (model.meshes.size() > 0) {
			// No nodes either, show all the meshes.
			for (const tinygltf::Mesh& mesh : model.meshes) {
				forEachPrimitiveInMeshObject(rootTransform, model, tinygltf::Node(), mesh, callback);
			}
		}
	}

}
