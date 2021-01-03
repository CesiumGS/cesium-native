#include "catch2/catch.hpp"
#include "CesiumUtility/Math.h"
#include "Cesium3DTiles/Gltf.h"
#include "Cesium3DTiles/GltfAccessor.h"
#include "CesiumUtility/Math.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "upsampleGltfForRasterOverlays.h"
#include "glm/trigonometric.hpp"
#include <vector>

using namespace Cesium3DTiles;
using namespace CesiumUtility;

TEST_CASE("Test upsample tile without skirts") {
	const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
	CesiumGeospatial::Cartographic bottomLeftCart{ glm::radians(110.0), glm::radians(32.0), 0.0 };
	CesiumGeospatial::Cartographic topLeftCart{ bottomLeftCart.longitude, bottomLeftCart.latitude + glm::radians(1.0), 0.0 };
	CesiumGeospatial::Cartographic topRightCart{ bottomLeftCart.longitude + glm::radians(1.0), bottomLeftCart.latitude + glm::radians(1.0), 0.0 };
	CesiumGeospatial::Cartographic bottomRightCart{ bottomLeftCart.longitude + glm::radians(1.0), bottomLeftCart.latitude, 0.0 };
	CesiumGeospatial::Cartographic centerCart{ (bottomLeftCart.longitude + topRightCart.longitude) / 2.0, (bottomLeftCart.latitude + topRightCart.latitude) / 2.0, 0.0 };
	glm::dvec3 center = ellipsoid.cartographicToCartesian(centerCart);
	std::vector<glm::vec3> positions{
		static_cast<glm::vec3>(ellipsoid.cartographicToCartesian(bottomLeftCart) - center),
		static_cast<glm::vec3>(ellipsoid.cartographicToCartesian(topLeftCart) - center),
		static_cast<glm::vec3>(ellipsoid.cartographicToCartesian(topRightCart) - center),
		static_cast<glm::vec3>(ellipsoid.cartographicToCartesian(bottomRightCart) - center),
	};
	std::vector<glm::vec2> uvs{
		glm::vec2{0.0, 0.0}, glm::vec2{0.0, 1.0},
		glm::vec2{1.0, 0.0}, glm::vec2{1.0, 1.0}
	};
	std::vector<uint16_t> indices{
		0, 2, 1,
		1, 2, 3
	};
	uint32_t positionsBufferSize = static_cast<uint32_t>(positions.size() * sizeof(glm::vec3));
	uint32_t uvsBufferSize = static_cast<uint32_t>(uvs.size() * sizeof(glm::vec2));
	uint32_t indicesBufferSize = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

	tinygltf::Model model;

	// create buffer
	model.buffers.emplace_back();
	tinygltf::Buffer &buffer = model.buffers.back();
	buffer.data.resize(positionsBufferSize + uvsBufferSize + indicesBufferSize);
	std::memcpy(buffer.data.data(), positions.data(), positionsBufferSize);
	std::memcpy(buffer.data.data() + positionsBufferSize, uvs.data(), uvsBufferSize);
	std::memcpy(buffer.data.data() + positionsBufferSize + uvsBufferSize, indices.data(), indicesBufferSize);

	// create position
	model.bufferViews.emplace_back();
	tinygltf::BufferView &positionBufferView = model.bufferViews.emplace_back();
	positionBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
	positionBufferView.byteOffset = 0;
	positionBufferView.byteLength = positionsBufferSize;
	positionBufferView.byteStride = 0;

	model.accessors.emplace_back();
	tinygltf::Accessor &positionAccessor = model.accessors.back();
	positionAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
	positionAccessor.byteOffset = 0;
	positionAccessor.count = positions.size();
	positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	positionAccessor.type = TINYGLTF_TYPE_VEC3;

	int positionAccessorIdx = static_cast<int>(model.accessors.size() - 1);

	// create uv
	model.bufferViews.emplace_back();
	tinygltf::BufferView &uvBufferView = model.bufferViews.emplace_back();
	uvBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
	uvBufferView.byteOffset = positionsBufferSize;
	uvBufferView.byteLength = uvsBufferSize;
	uvBufferView.byteStride = 0;

	model.accessors.emplace_back();
	tinygltf::Accessor &uvAccessor = model.accessors.back();
	uvAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
	uvAccessor.byteOffset = 0;
	uvAccessor.count = uvs.size();
	uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	uvAccessor.type = TINYGLTF_TYPE_VEC2;

	int uvAccessorIdx = static_cast<int>(model.accessors.size() - 1);

	// create indices
	model.bufferViews.emplace_back();
	tinygltf::BufferView &indicesBufferView = model.bufferViews.emplace_back();
	indicesBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
	indicesBufferView.byteOffset = positionsBufferSize + uvsBufferSize;
	indicesBufferView.byteLength = indicesBufferSize;
	indicesBufferView.byteStride = 0;

	model.accessors.emplace_back();
	tinygltf::Accessor &indicesAccessor = model.accessors.back();
	indicesAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
	indicesAccessor.byteOffset = 0;
	indicesAccessor.count = indices.size();
	indicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	indicesAccessor.type = TINYGLTF_TYPE_SCALAR;

	int indicesAccessorIdx = static_cast<int>(model.accessors.size() - 1);

	model.meshes.emplace_back();
	tinygltf::Mesh &mesh = model.meshes.back();
	mesh.primitives.emplace_back();

	tinygltf::Primitive &primitive = mesh.primitives.back();
	primitive.mode = TINYGLTF_MODE_TRIANGLES;
	primitive.attributes["_CESIUMOVERLAY_0"] = uvAccessorIdx;
	primitive.attributes["POSITION"] = positionAccessorIdx;
	primitive.indices = indicesAccessorIdx;

	// create node and update bounding volume
	model.nodes.emplace_back();
	tinygltf::Node& node = model.nodes[0];
	node.mesh = static_cast<int>(model.meshes.size() - 1);
	node.matrix = {
		1.0, 0.0,  0.0, 0.0,
		0.0, 0.0, -1.0, 0.0,
		0.0, 1.0,  0.0, 0.0,
		center.x, center.z, -center.y, 1.0
	};

	SECTION("Upsample bottom left child") {
		tinygltf::Model upsampledModel = upsampleGltfForRasterOverlays(model, CesiumGeometry::QuadtreeChild::LowerLeft);

		REQUIRE(upsampledModel.meshes.size() == 1);
		const tinygltf::Mesh& upsampledMesh = upsampledModel.meshes.back();

		REQUIRE(upsampledMesh.primitives.size() == 1);
		const tinygltf::Primitive& upsampledPrimitive = upsampledMesh.primitives.back();

		REQUIRE(upsampledPrimitive.indices >= 0);
		REQUIRE(upsampledPrimitive.attributes.find("POSITION") != upsampledPrimitive.attributes.end());
		GltfAccessor<glm::vec3> upsampledPosition(upsampledModel, static_cast<size_t>(upsampledPrimitive.attributes.at("POSITION")));
		GltfAccessor<uint32_t> upsampledIndices(upsampledModel, static_cast<size_t>(upsampledPrimitive.indices));

		glm::vec3 p0 = upsampledPosition[0];
		REQUIRE(glm::epsilonEqual(p0, positions[0], glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p1 = upsampledPosition[1];
		REQUIRE(glm::epsilonEqual(p1, (positions[0] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p2 = upsampledPosition[2];
		REQUIRE(glm::epsilonEqual(p2, (upsampledPosition[1] + positions[1]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p3 = upsampledPosition[3];
		REQUIRE(glm::epsilonEqual(p3, (positions[0] + positions[1]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p4 = upsampledPosition[4];
		REQUIRE(glm::epsilonEqual(p4, (positions[0] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p5 = upsampledPosition[5];
		REQUIRE(glm::epsilonEqual(p5, (positions[1] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p6 = upsampledPosition[6];
		REQUIRE(glm::epsilonEqual(p6, (upsampledPosition[4] + positions[1]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));
	}

	SECTION("Upsample upper left child") {
		tinygltf::Model upsampledModel = upsampleGltfForRasterOverlays(model, CesiumGeometry::QuadtreeChild::UpperLeft);

		REQUIRE(upsampledModel.meshes.size() == 1);
		const tinygltf::Mesh& upsampledMesh = upsampledModel.meshes.back();

		REQUIRE(upsampledMesh.primitives.size() == 1);
		const tinygltf::Primitive& upsampledPrimitive = upsampledMesh.primitives.back();

		REQUIRE(upsampledPrimitive.indices >= 0);
		REQUIRE(upsampledPrimitive.attributes.find("POSITION") != upsampledPrimitive.attributes.end());
		GltfAccessor<glm::vec3> upsampledPosition(upsampledModel, static_cast<size_t>(upsampledPrimitive.attributes.at("POSITION")));
		GltfAccessor<uint32_t> upsampledIndices(upsampledModel, static_cast<size_t>(upsampledPrimitive.indices));

		glm::vec3 p0 = upsampledPosition[0];
		REQUIRE(glm::epsilonEqual(p0, positions[1], glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p1 = upsampledPosition[1];
		REQUIRE(glm::epsilonEqual(p1, (positions[0] + positions[1]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p2 = upsampledPosition[2];
		REQUIRE(glm::epsilonEqual(p2, (positions[1] + 0.5f * (positions[0] + positions[2])) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p3 = upsampledPosition[3];
		REQUIRE(glm::epsilonEqual(p3, (positions[1] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p4 = upsampledPosition[4];
		REQUIRE(glm::epsilonEqual(p4, p2, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p5 = upsampledPosition[5];
		REQUIRE(glm::epsilonEqual(p5, (positions[1] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p6 = upsampledPosition[6];
		REQUIRE(glm::epsilonEqual(p6, (positions[1] + positions[3]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));
	}

	SECTION("Upsample bottom right child") {
		tinygltf::Model upsampledModel = upsampleGltfForRasterOverlays(model, CesiumGeometry::QuadtreeChild::LowerRight);

		REQUIRE(upsampledModel.meshes.size() == 1);
		const tinygltf::Mesh& upsampledMesh = upsampledModel.meshes.back();

		REQUIRE(upsampledMesh.primitives.size() == 1);
		const tinygltf::Primitive& upsampledPrimitive = upsampledMesh.primitives.back();

		REQUIRE(upsampledPrimitive.indices >= 0);
		REQUIRE(upsampledPrimitive.attributes.find("POSITION") != upsampledPrimitive.attributes.end());
		GltfAccessor<glm::vec3> upsampledPosition(upsampledModel, static_cast<size_t>(upsampledPrimitive.attributes.at("POSITION")));
		GltfAccessor<uint32_t> upsampledIndices(upsampledModel, static_cast<size_t>(upsampledPrimitive.indices));

		glm::vec3 p0 = upsampledPosition[0];
		REQUIRE(glm::epsilonEqual(p0, positions[2], glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p1 = upsampledPosition[1];
		REQUIRE(glm::epsilonEqual(p1, (positions[1] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p2 = upsampledPosition[2];
		REQUIRE(glm::epsilonEqual(p2, (positions[0] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p3 = upsampledPosition[3];
		REQUIRE(glm::epsilonEqual(p3, (positions[2] + positions[3]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p4 = upsampledPosition[4];
		REQUIRE(glm::epsilonEqual(p4, (positions[2] + (positions[1] + positions[3]) * 0.5f) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p5 = upsampledPosition[5];
		REQUIRE(glm::epsilonEqual(p5, (positions[1] + positions[2]) * 0.5f, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));

		glm::vec3 p6 = upsampledPosition[6];
		REQUIRE(glm::epsilonEqual(p6, p4, glm::vec3(static_cast<float>(Math::EPSILON7))) == glm::bvec3(true));
	}
}

