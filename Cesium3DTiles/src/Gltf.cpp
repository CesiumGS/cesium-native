#include "Cesium3DTiles/Gltf.h"

namespace Cesium3DTiles {
    Gltf::LoadResult Gltf::load(const gsl::span<const uint8_t>& data)
    {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string errors;
        std::string warnings;

        loader.LoadBinaryFromMemory(&model, &errors, &warnings, data.data(), static_cast<unsigned int>(data.size()));

        return LoadResult{
            model,
            warnings,
            errors
        };
    }

	/*static*/ void Gltf::forEachPrimitiveInScene(tinygltf::Model& gltf, int sceneID, std::function<ForEachPrimitiveInSceneCallback>&& callback) {
		return Gltf::forEachPrimitiveInScene(const_cast<const tinygltf::Model&>(gltf), sceneID, [&callback](
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
		std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback
	) {
		for (const tinygltf::Primitive& primitive : mesh.primitives) {
			callback(model, node, mesh, primitive, transform);
		}
	}

	static void forEachPrimitiveInNodeObject(const glm::dmat4x4& transform, const tinygltf::Model& model, const tinygltf::Node& node, std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {
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

	static void forEachPrimitiveInSceneObject(const glm::dmat4x4& transform, const tinygltf::Model& model, const tinygltf::Scene& scene, std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {
		for (int nodeID : scene.nodes) {
			if (nodeID >= 0 && nodeID < static_cast<int>(model.nodes.size())) {
				forEachPrimitiveInNodeObject(transform, model, model.nodes[nodeID], callback);
			}
		}
	}

	/*static*/ void Gltf::forEachPrimitiveInScene(const tinygltf::Model& gltf, int sceneID, std::function<Gltf::ForEachPrimitiveInSceneConstCallback>&& callback) {
		glm::dmat4x4 rootTransform = gltfAxesToCesiumAxes;

		if (sceneID >= 0) {
			// Use the user-specified scene if it exists.
			if (sceneID < static_cast<int>(gltf.scenes.size())) {
				forEachPrimitiveInSceneObject(rootTransform, gltf, gltf.scenes[sceneID], callback);
			}
		} else if (gltf.defaultScene >= 0 && gltf.defaultScene < static_cast<int>(gltf.scenes.size())) {
			// Use the default scene
			forEachPrimitiveInSceneObject(rootTransform, gltf, gltf.scenes[gltf.defaultScene], callback);
		} else if (gltf.scenes.size() > 0) {
			// There's no default, so use the first scene
			const tinygltf::Scene& defaultScene = gltf.scenes[0];
			forEachPrimitiveInSceneObject(rootTransform, gltf, defaultScene, callback);
		} else if (gltf.nodes.size() > 0) {
			// No scenes at all, use the first node as the root node.
			forEachPrimitiveInNodeObject(rootTransform, gltf, gltf.nodes[0], callback);
		} else if (gltf.meshes.size() > 0) {
			// No nodes either, show all the meshes.
			for (const tinygltf::Mesh& mesh : gltf.meshes) {
				forEachPrimitiveInMeshObject(rootTransform, gltf, tinygltf::Node(), mesh, callback);
			}
		}
	}

}
