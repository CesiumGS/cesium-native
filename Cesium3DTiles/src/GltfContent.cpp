#include "Cesium3DTiles/GltfAccessor.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/GltfWriter.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumUtility/Math.h"
#include <stdexcept>
#include <chrono>

namespace Cesium3DTiles {

	static std::atomic<double> maxTime;

	/*static*/ std::unique_ptr<TileContentLoadResult> GltfContent::load(
		std::shared_ptr<spdlog::logger> pLogger,
		const TileContext& /*context*/,
		const TileID& /*tileID*/,
		const BoundingVolume& /*tileBoundingVolume*/,
		double /*tileGeometricError*/,
		const glm::dmat4& /*tileTransform*/,
		const std::optional<BoundingVolume>& /*tileContentBoundingVolume*/,
		TileRefine /*tileRefine*/,
		const std::string& /*url*/,
		const gsl::span<const uint8_t>& data
	) {
		std::unique_ptr<TileContentLoadResult> pResult = std::make_unique<TileContentLoadResult>();

		// auto startTime = std::chrono::high_resolution_clock::now();

		std::string errors;
		std::string warnings;

		tinygltf::TinyGLTF loader;
		pResult->model.emplace();
		bool success = loader.LoadBinaryFromMemory(&pResult->model.value(), &errors, &warnings, data.data(), static_cast<unsigned int>(data.size()));
		if (!success) {
			SPDLOG_LOGGER_ERROR(pLogger, "Failed to load binary glTF from memory: {}", errors);
		}
		if (!warnings.empty()) {
			SPDLOG_LOGGER_WARN(pLogger, "Warning when loading binary glTF from memory: {}", warnings);
		}

		// auto endTime = std::chrono::high_resolution_clock::now();
		// double t = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime).count();
		// double m = std::max(maxTime.load(), t);
		// while (maxTime.exchange(m) > m) {
		// 	m = std::max(maxTime.load(), t);
		// }

		// SPDLOG_LOGGER_WARN(pLogger, "{} loaded in {} seconds, max seen {}", url, t, m);

		return pResult;
	}

	static int generateOverlayTextureCoordinates(
		tinygltf::Model& gltf,
		int positionAccessorIndex,
		const glm::dmat4x4& transform,
		const CesiumGeospatial::Projection& projection,
		const CesiumGeometry::Rectangle& rectangle,
		double& west,
		double& south,
		double& east,
		double& north,
		double& minimumHeight,
		double& maximumHeight
	) {
        int uvBufferId = static_cast<int>(gltf.buffers.size());
        gltf.buffers.emplace_back();

        int uvBufferViewId = static_cast<int>(gltf.bufferViews.size());
        gltf.bufferViews.emplace_back();

        int uvAccessorId = static_cast<int>(gltf.accessors.size());
        gltf.accessors.emplace_back();

		GltfAccessor<glm::vec3> positionAccessor(gltf, static_cast<size_t>(positionAccessorIndex));

        tinygltf::Buffer& uvBuffer = gltf.buffers[static_cast<size_t>(uvBufferId)];
        uvBuffer.data.resize(positionAccessor.size() * 2 * sizeof(float));

        tinygltf::BufferView& uvBufferView = gltf.bufferViews[static_cast<size_t>(uvBufferViewId)];
        uvBufferView.buffer = uvBufferId;
        uvBufferView.byteOffset = 0;
        uvBufferView.byteStride = 2 * sizeof(float);
        uvBufferView.byteLength = uvBuffer.data.size();
        uvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

        tinygltf::Accessor& uvAccessor = gltf.accessors[static_cast<size_t>(uvAccessorId)];
        uvAccessor.bufferView = uvBufferViewId;
        uvAccessor.byteOffset = 0;
        uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        uvAccessor.count = positionAccessor.size();
        uvAccessor.type = TINYGLTF_TYPE_VEC2;

		GltfWriter<glm::vec2> uvWriter(gltf, static_cast<size_t>(uvAccessorId));

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

			double longitude = cartographic.value().longitude;
			double latitude = cartographic.value().latitude;
			double ellipsoidHeight = cartographic.value().height;

			// If the position is near the anti-meridian and the projected position is outside the expected range, try
			// using the equivalent longitude on the other side of the anti-meridian to see if that gets us closer.
			if (
				glm::abs(glm::abs(cartographic.value().longitude) - CesiumUtility::Math::ONE_PI) < CesiumUtility::Math::EPSILON5 &&
				(
					projectedPosition.x < rectangle.minimumX ||
					projectedPosition.x > rectangle.maximumX ||
					projectedPosition.y < rectangle.minimumY ||
					projectedPosition.y > rectangle.maximumY
				)
			) {
				cartographic.value().longitude += cartographic.value().longitude < 0.0 ? CesiumUtility::Math::TWO_PI : -CesiumUtility::Math::TWO_PI;
				glm::dvec3 projectedPosition2 = projectPosition(projection, cartographic.value());
				
				double distance1 = rectangle.computeSignedDistance(projectedPosition);
				double distance2 = rectangle.computeSignedDistance(projectedPosition2);

				if (distance2 < distance1) {
					projectedPosition = projectedPosition2;
					longitude = cartographic.value().longitude;
				}
			}

			// The computation of longitude is very unstable at the poles,
			// so don't let extreme latitudes affect the longitude bounding box.
			if (glm::abs(glm::abs(latitude) - CesiumUtility::Math::PI_OVER_TWO) > CesiumUtility::Math::EPSILON6) {
				west = glm::min(west, longitude);
				east = glm::max(east, longitude);
			}
			south = glm::min(south, latitude);
			north = glm::max(north, latitude);
			minimumHeight = glm::min(minimumHeight, ellipsoidHeight);
			maximumHeight = glm::max(maximumHeight, ellipsoidHeight);

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

	/*static*/ CesiumGeospatial::BoundingRegion GltfContent::createRasterOverlayTextureCoordinates(
		tinygltf::Model& gltf,
		uint32_t textureCoordinateID,
		const CesiumGeospatial::Projection& projection,
		const CesiumGeometry::Rectangle& rectangle
	) {
		std::vector<int> positionAccessorsToTextureCoordinateAccessor;
		positionAccessorsToTextureCoordinateAccessor.resize(gltf.accessors.size(), 0);

		std::string attributeName = "_CESIUMOVERLAY_" + std::to_string(textureCoordinateID);

		double west = CesiumUtility::Math::ONE_PI;
		double south = CesiumUtility::Math::PI_OVER_TWO;
		double east = -CesiumUtility::Math::ONE_PI;
		double north = -CesiumUtility::Math::PI_OVER_TWO;
		double minimumHeight = std::numeric_limits<double>::max();
		double maximumHeight = std::numeric_limits<double>::lowest();

		Gltf::forEachPrimitiveInScene(gltf, -1, [&positionAccessorsToTextureCoordinateAccessor, &attributeName, &projection, &rectangle, &west, &south, &east, &north, &minimumHeight, &maximumHeight](
            tinygltf::Model& gltf_,
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
			if (positionAccessorIndex < 0 || positionAccessorIndex >= static_cast<int>(gltf_.accessors.size())) {
				return;
			}

			int textureCoordinateAccessorIndex = positionAccessorsToTextureCoordinateAccessor[static_cast<size_t>(positionAccessorIndex)];
			if (textureCoordinateAccessorIndex > 0) {
				primitive.attributes[attributeName] = textureCoordinateAccessorIndex;
				return;
			}

			// TODO remove this check
			if (primitive.attributes.find(attributeName) != primitive.attributes.end()) {
				return;
			}

			// Generate new texture coordinates
			int nextTextureCoordinateAccessorIndex = generateOverlayTextureCoordinates(gltf_, positionAccessorIndex, transform, projection, rectangle, west, south, east, north, minimumHeight, maximumHeight);
			primitive.attributes[attributeName] = nextTextureCoordinateAccessorIndex;
			positionAccessorsToTextureCoordinateAccessor[static_cast<size_t>(positionAccessorIndex)] = nextTextureCoordinateAccessorIndex;
		});

		return CesiumGeospatial::BoundingRegion(
			CesiumGeospatial::GlobeRectangle(west, south, east, north),
			minimumHeight,
			maximumHeight
		);
	}

}
