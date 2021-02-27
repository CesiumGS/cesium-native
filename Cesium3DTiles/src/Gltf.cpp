// Copyright CesiumGS, Inc. and Contributors

#include "Cesium3DTiles/Gltf.h"

namespace Cesium3DTiles {
	/*static*/ void Gltf::forEachPrimitiveInScene(CesiumGltf::Model& gltf, int sceneID, std::function<ForEachPrimitiveInSceneCallback>&& callback) {
		return Gltf::forEachPrimitiveInScene(const_cast<const CesiumGltf::Model&>(gltf), sceneID, [&callback](
            const CesiumGltf::Model& gltf_,
			const CesiumGltf::Node& node,
            const CesiumGltf::Mesh& mesh,
            const CesiumGltf::MeshPrimitive& primitive,
            const glm::dmat4& transform
		) {
			callback(
				const_cast<CesiumGltf::Model&>(gltf_),
				const_cast<CesiumGltf::Node&>(node),
				const_cast<CesiumGltf::Mesh&>(mesh),
				const_cast<CesiumGltf::MeshPrimitive&>(primitive),
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
		const CesiumGltf::Model& model,
		const CesiumGltf::Node& node,
		const CesiumGltf::Mesh& mesh,
		std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback
	) {
		for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
			callback(model, node, mesh, primitive, transform);
		}
	}

	static void forEachPrimitiveInNodeObject(const glm::dmat4x4& transform, const CesiumGltf::Model& model, const CesiumGltf::Node& node, std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {
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
			const CesiumGltf::Mesh& mesh = model.meshes[static_cast<size_t>(meshId)];
			forEachPrimitiveInMeshObject(nodeTransform, model, node, mesh, callback);
		}

		for (int childNodeId : node.children) {
			if (childNodeId >= 0 && childNodeId < static_cast<int>(model.nodes.size())) {
				forEachPrimitiveInNodeObject(nodeTransform, model, model.nodes[static_cast<size_t>(childNodeId)], callback);
			}
		}
	}

	static void forEachPrimitiveInSceneObject(const glm::dmat4x4& transform, const CesiumGltf::Model& model, const CesiumGltf::Scene& scene, std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {
		for (int nodeID : scene.nodes) {
			if (nodeID >= 0 && nodeID < static_cast<int>(model.nodes.size())) {
				forEachPrimitiveInNodeObject(transform, model, model.nodes[static_cast<size_t>(nodeID)], callback);
			}
		}
	}

	/*static*/ void Gltf::forEachPrimitiveInScene(const CesiumGltf::Model& gltf, int sceneID, std::function<Gltf::ForEachPrimitiveInSceneConstCallback>&& callback) {
		glm::dmat4x4 rootTransform = gltfAxesToCesiumAxes;

		if (sceneID >= 0) {
			// Use the user-specified scene if it exists.
			if (sceneID < static_cast<int>(gltf.scenes.size())) {
				forEachPrimitiveInSceneObject(rootTransform, gltf, gltf.scenes[static_cast<size_t>(sceneID)], callback);
			}
		} else if (gltf.scene >= 0 && gltf.scene < static_cast<int32_t>(gltf.scenes.size())) {
			// Use the default scene
			forEachPrimitiveInSceneObject(rootTransform, gltf, gltf.scenes[static_cast<size_t>(gltf.scene)], callback);
		} else if (gltf.scenes.size() > 0) {
			// There's no default, so use the first scene
			const CesiumGltf::Scene& defaultScene = gltf.scenes[0];
			forEachPrimitiveInSceneObject(rootTransform, gltf, defaultScene, callback);
		} else if (gltf.nodes.size() > 0) {
			// No scenes at all, use the first node as the root node.
			forEachPrimitiveInNodeObject(rootTransform, gltf, gltf.nodes[0], callback);
		} else if (gltf.meshes.size() > 0) {
			// No nodes either, show all the meshes.
			for (const CesiumGltf::Mesh& mesh : gltf.meshes) {
				forEachPrimitiveInMeshObject(rootTransform, gltf, CesiumGltf::Node(), mesh, callback);
			}
		}
	}

}
