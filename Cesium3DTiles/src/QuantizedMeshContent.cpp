#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "CesiumGeometry/QuadtreeTileRectangularRange.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumUtility/Math.h"
#include "QuantizedMeshContent.h"
#include "tiny_gltf.h"
#include "Uri.h"
#include <glm/vec3.hpp>
#include <stdexcept>

using namespace CesiumUtility;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;

namespace Cesium3DTiles {
    /*static*/ std::string QuantizedMeshContent::CONTENT_TYPE = "application/vnd.quantized-mesh";

    struct QuantizedMeshHeader
    {
        // The center of the tile in Earth-centered Fixed coordinates.
        double CenterX;
        double CenterY;
        double CenterZ;

        // The minimum and maximum heights in the area covered by this tile.
        // The minimum may be lower and the maximum may be higher than
        // the height of any vertex in this tile in the case that the min/max vertex
        // was removed during mesh simplification, but these are the appropriate
        // values to use for analysis or visualization.
        float MinimumHeight;
        float MaximumHeight;

        // The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
        // in Earth-centered Fixed coordinates, and the radius is in meters.
        double BoundingSphereCenterX;
        double BoundingSphereCenterY;
        double BoundingSphereCenterZ;
        double BoundingSphereRadius;

        // The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
        // If this point is below the horizon, the entire tile is below the horizon.
        // See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
        double HorizonOcclusionPointX;
        double HorizonOcclusionPointY;
        double HorizonOcclusionPointZ;

        // The total number of vertices.
        uint32_t vertexCount;
    };

    struct ExtensionHeader
    {
        uint8_t extensionId;
        uint32_t extensionLength;
    };

    // We can't use sizeof(QuantizedMeshHeader) because it may be padded.
    const size_t headerLength = 92;
    const size_t extensionHeaderLength = 5;

    int32_t zigZagDecode(int32_t value) {
        return (value >> 1) ^ (-(value & 1));
    }

    template <class T, class D>
    void decodeIndices(const gsl::span<const T>& encoded, gsl::span<D>& decoded) {
        if (decoded.size() < encoded.size()) {
            throw std::runtime_error("decoded buffer is too small.");
        }

        T highest = 0;
        for (size_t i = 0; i < encoded.size(); ++i) {
            T code = encoded[i];
            decoded[i] = static_cast<D>(highest - code);
            if (code == 0) {
                ++highest;
            }
        }
    }

    template <class T>
    static T readValue(const gsl::span<const uint8_t>& data, size_t offset, T defaultValue) {
        if (offset + sizeof(T) <= data.size()) {
            return *reinterpret_cast<const T*>(data.data() + offset);
        }
        return defaultValue;
    }

	static double calculateSkirtHeight(int tileLevel, const CesiumGeospatial::Ellipsoid &ellipsoid, const QuadtreeTilingScheme &tilingScheme) {
		static const double terrainHeightmapQuality = 0.25;
		static const uint32_t heightmapWidth = 65;
		double levelZeroMaximumGeometricError = ellipsoid.getMaximumRadius() * CesiumUtility::Math::TWO_PI * terrainHeightmapQuality
			/ (heightmapWidth * tilingScheme.getRootTilesX());

		double levelMaximumGeometricError = levelZeroMaximumGeometricError / (1 << tileLevel);
		return levelMaximumGeometricError * 5.0;
	}

    template <class E, class I>
	static void addSkirt(
		const CesiumGeospatial::Ellipsoid &ellipsoid,
		glm::dvec3 center,
		const CesiumGeospatial::GlobeRectangle &rectangle,
		double minimumHeight,
		double maximumHeight,
		uint32_t currentVertexCount,
		uint32_t currentIndicesCount,
		double skirtHeight,
		double longitudeOffset,
		double latitudeOffset,
		const std::vector<glm::dvec3>& uvsAndHeights,
        const gsl::span<const E> &edgeIndices,
        gsl::span<float> &positions,
		gsl::span<float> &normals,
        gsl::span<I> &indices) 
    {
        double west = rectangle.getWest();
        double south = rectangle.getSouth();
        double east = rectangle.getEast();
        double north = rectangle.getNorth();

        size_t newEdgeIndex = currentVertexCount;
        size_t positionIdx = currentVertexCount * 3;
        size_t indexIdx = currentIndicesCount;
        for (size_t i = 0; i < edgeIndices.size() - 1; ++i) {
            E edgeIdx = edgeIndices[i];
            E nextEdgeIdx = edgeIndices[i + 1];

			double uRatio = uvsAndHeights[edgeIdx].x;
			double vRatio = uvsAndHeights[edgeIdx].y;
			double heightRatio = uvsAndHeights[edgeIdx].z;
			double longitude = Math::lerp(west, east, uRatio) + longitudeOffset;
			double latitude = Math::lerp(south, north, vRatio) + latitudeOffset;
			double heightMeters = Math::lerp(minimumHeight, maximumHeight, heightRatio) - skirtHeight;
			glm::dvec3 position = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, heightMeters));
			position -= center;

            positions[positionIdx] = static_cast<float>(position.x);
            positions[positionIdx + 1] = static_cast<float>(position.y);
            positions[positionIdx + 2] = static_cast<float>(position.z);

			if (!normals.empty()) {
				normals[positionIdx] = normals[3 * edgeIdx];
				normals[positionIdx + 1] = normals[3 * edgeIdx + 1];
				normals[positionIdx + 2] = normals[3 * edgeIdx + 2];
			}

            indices[indexIdx++] = static_cast<I>(edgeIdx);
            indices[indexIdx++] = static_cast<I>(nextEdgeIdx);
            indices[indexIdx++] = static_cast<I>(newEdgeIndex);

            indices[indexIdx++] = static_cast<I>(newEdgeIndex);
            indices[indexIdx++] = static_cast<I>(nextEdgeIdx);
            indices[indexIdx++] = static_cast<I>(newEdgeIndex + 1);

            ++newEdgeIndex;
			positionIdx += 3;
        }

		E edgeIdx = edgeIndices.back();
		double uRatio = uvsAndHeights[edgeIdx].x;
		double vRatio = uvsAndHeights[edgeIdx].y;
		double heightRatio = uvsAndHeights[edgeIdx].z;
		double longitude = Math::lerp(west, east, uRatio) + longitudeOffset;
		double latitude = Math::lerp(south, north, vRatio) + latitudeOffset;
		double heightMeters = Math::lerp(minimumHeight, maximumHeight, heightRatio) - skirtHeight;
		glm::dvec3 position = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, heightMeters));
		position -= center;

		positions[positionIdx] = static_cast<float>(position.x);
		positions[positionIdx + 1] = static_cast<float>(position.y);
		positions[positionIdx + 2] = static_cast<float>(position.z);
    }

    template <class E, class I>
	static void addSkirts(
		const CesiumGeospatial::Ellipsoid &ellipsoid,
		glm::dvec3 center,
		const CesiumGeospatial::GlobeRectangle &rectangle,
		double minimumHeight,
		double maximumHeight,
		uint32_t currentVertexCount,
		uint32_t currentIndicesCount,
		double skirtHeight,
		double longitudeOffset,
		double latitudeOffset,
		const std::vector<glm::dvec3> &uvsAndHeights,
		const gsl::span<const uint8_t> &westEdgeIndicesBuffer,
		const gsl::span<const uint8_t> &southEdgeIndicesBuffer,
		const gsl::span<const uint8_t> &eastEdgeIndicesBuffer,
		const gsl::span<const uint8_t> &northEdgeIndicesBuffer,
		gsl::span<float>& outputPositions,
		gsl::span<float>& outputNormals,
		gsl::span<I>& outputIndices) 
	{
		uint32_t westVertexCount = static_cast<uint32_t>(westEdgeIndicesBuffer.size() / sizeof(E));
		uint32_t southVertexCount = static_cast<uint32_t>(southEdgeIndicesBuffer.size() / sizeof(E));
		uint32_t eastVertexCount = static_cast<uint32_t>(eastEdgeIndicesBuffer.size() / sizeof(E));
		uint32_t northVertexCount = static_cast<uint32_t>(northEdgeIndicesBuffer.size() / sizeof(E));

		// allocate edge indices to be sort later
		uint32_t maxEdgeVertexCount = westVertexCount;
		maxEdgeVertexCount = glm::max(maxEdgeVertexCount, southVertexCount);
		maxEdgeVertexCount = glm::max(maxEdgeVertexCount, eastVertexCount);
		maxEdgeVertexCount = glm::max(maxEdgeVertexCount, northVertexCount);
		std::vector<E> sortEdgeIndices(maxEdgeVertexCount);

		// add skirt indices, vertices, and normals
		gsl::span<const E> westEdgeIndices(reinterpret_cast<const E*>(westEdgeIndicesBuffer.data()), westVertexCount);
		std::partial_sort_copy(westEdgeIndices.begin(), 
			westEdgeIndices.end(), 
			sortEdgeIndices.begin(), 
			sortEdgeIndices.begin() + westVertexCount, 
			[&uvsAndHeights](auto lhs, auto rhs) { return uvsAndHeights[lhs].y < uvsAndHeights[rhs].y;  });
		westEdgeIndices = gsl::span(sortEdgeIndices.data(), westVertexCount);
		addSkirt(ellipsoid,
			center,
			rectangle,
			minimumHeight,
			maximumHeight,
			currentVertexCount, 
			currentIndicesCount, 
			skirtHeight, 
			longitudeOffset,
			latitudeOffset,
			uvsAndHeights,
			westEdgeIndices, 
			outputPositions, 
			outputNormals, 
			outputIndices);

		currentVertexCount += westVertexCount;
		currentIndicesCount += (westVertexCount - 1) * 6;
		gsl::span<const E> southEdgeIndices(reinterpret_cast<const E*>(southEdgeIndicesBuffer.data()), southVertexCount);
		std::partial_sort_copy(southEdgeIndices.begin(), 
			southEdgeIndices.end(), 
			sortEdgeIndices.begin(), 
			sortEdgeIndices.begin() + southVertexCount, 
			[&uvsAndHeights](auto lhs, auto rhs) { return uvsAndHeights[lhs].x > uvsAndHeights[rhs].x;  });
		southEdgeIndices = gsl::span(sortEdgeIndices.data(), southVertexCount);
		addSkirt(ellipsoid, 
			center,
			rectangle,
			minimumHeight,
			maximumHeight, 
			currentVertexCount, 
			currentIndicesCount, 
			skirtHeight, 
			longitudeOffset,
			latitudeOffset,
			uvsAndHeights,
			southEdgeIndices, 
			outputPositions, 
			outputNormals, 
			outputIndices);

		currentVertexCount += southVertexCount;
		currentIndicesCount += (southVertexCount - 1) * 6;
		gsl::span<const E> eastEdgeIndices(reinterpret_cast<const E*>(eastEdgeIndicesBuffer.data()), eastVertexCount);
		std::partial_sort_copy(eastEdgeIndices.begin(), 
			eastEdgeIndices.end(), 
			sortEdgeIndices.begin(), 
			sortEdgeIndices.begin() + eastVertexCount, 
			[&uvsAndHeights](auto lhs, auto rhs) { return uvsAndHeights[lhs].y > uvsAndHeights[rhs].y;  });
		eastEdgeIndices = gsl::span(sortEdgeIndices.data(), eastVertexCount);
		addSkirt(ellipsoid, 
			center, 
			rectangle, 
			minimumHeight, 
			maximumHeight, 
			currentVertexCount, 
			currentIndicesCount, 
			skirtHeight, 
			longitudeOffset,
			latitudeOffset,
			uvsAndHeights,
			eastEdgeIndices, 
			outputPositions, 
			outputNormals, 
			outputIndices);

		currentVertexCount += eastVertexCount;
		currentIndicesCount += (eastVertexCount - 1) * 6;
		gsl::span<const E> northEdgeIndices(reinterpret_cast<const E*>(northEdgeIndicesBuffer.data()), northVertexCount);
		std::partial_sort_copy(northEdgeIndices.begin(), 
			northEdgeIndices.end(), 
			sortEdgeIndices.begin(), 
			sortEdgeIndices.begin() + northVertexCount, 
			[&uvsAndHeights](auto lhs, auto rhs) { return uvsAndHeights[lhs].x < uvsAndHeights[rhs].x; });
		northEdgeIndices = gsl::span(sortEdgeIndices.data(), northVertexCount);
		addSkirt(ellipsoid,
			center,
			rectangle, 
			minimumHeight, 
			maximumHeight, 
			currentVertexCount, 
			currentIndicesCount, 
			skirtHeight, 
			longitudeOffset,
			latitudeOffset,
			uvsAndHeights,
			northEdgeIndices, 
			outputPositions, 
			outputNormals, 
			outputIndices);
	}

    static glm::dvec3 octDecode(uint8_t x, uint8_t y) {
        const uint8_t rangeMax = 255;

        glm::dvec3 result;

        result.x = CesiumUtility::Math::fromSNorm(x, rangeMax);
        result.y = CesiumUtility::Math::fromSNorm(y, rangeMax);
        result.z = 1.0 - (glm::abs(result.x) + glm::abs(result.y));

        if (result.z < 0.0) {
            double oldVX = result.x;
            result.x = (1.0 - glm::abs(result.y)) * CesiumUtility::Math::signNotZero(oldVX);
            result.y = (1.0 - glm::abs(oldVX)) * CesiumUtility::Math::signNotZero(result.y);
        }

        return glm::normalize(result);
    }

    static void processMetadata(const QuadtreeTileID& tileID, gsl::span<const char> json, TileContentLoadResult& result);

    /*static*/ std::unique_ptr<TileContentLoadResult> QuantizedMeshContent::load(
        const TileContext& context,
        const TileID& tileID,
        const BoundingVolume& tileBoundingVolume,
        double /*tileGeometricError*/,
        const glm::dmat4& /*tileTransform*/,
        const std::optional<BoundingVolume>& /*tileContentBoundingVolume*/,
        TileRefine /*tileRefine*/,
        const std::string& /*url*/,
        const gsl::span<const uint8_t>& data
    ) {
        // TODO: use context plus tileID to compute the tile's rectangle, rather than inferring it from the parent tile.
        const QuadtreeTileID& id = std::get<QuadtreeTileID>(tileID);

        std::unique_ptr<TileContentLoadResult> pResult = std::make_unique<TileContentLoadResult>();

        if (data.size() < headerLength) {
            return pResult;
        }

        size_t readIndex = 0;

        const QuantizedMeshHeader* pHeader = reinterpret_cast<const QuantizedMeshHeader*>(data.data());
        glm::dvec3 center(pHeader->BoundingSphereCenterX, pHeader->BoundingSphereCenterY, pHeader->BoundingSphereCenterZ);
        glm::dvec3 horizonOcclusionPoint(pHeader->HorizonOcclusionPointX, pHeader->HorizonOcclusionPointY, pHeader->HorizonOcclusionPointZ);
        double minimumHeight = pHeader->MinimumHeight;
        double maximumHeight = pHeader->MaximumHeight;

        uint32_t vertexCount = pHeader->vertexCount;

        readIndex += headerLength;

        const gsl::span<const uint16_t> uBuffer(reinterpret_cast<const uint16_t*>(data.data() + readIndex), vertexCount);
        readIndex += uBuffer.size_bytes();
        if (readIndex > data.size()) {
            return pResult;
        }

        const gsl::span<const uint16_t> vBuffer(reinterpret_cast<const uint16_t*>(data.data() + readIndex), vertexCount);
        readIndex += vBuffer.size_bytes();
        if (readIndex > data.size()) {
            return pResult;
        }

        const gsl::span<const uint16_t> heightBuffer(reinterpret_cast<const uint16_t*>(data.data() + readIndex), vertexCount);
        readIndex += heightBuffer.size_bytes();
        if (readIndex > data.size()) {
            return pResult;
        }

        const BoundingRegion* pRegion = std::get_if<BoundingRegion>(&tileBoundingVolume);
        if (!pRegion) {
            const BoundingRegionWithLooseFittingHeights* pLooseRegion = std::get_if<BoundingRegionWithLooseFittingHeights>(&tileBoundingVolume);
            if (pLooseRegion) {
                pRegion = &pLooseRegion->getBoundingRegion();
            }
        }

        if (!pRegion) {
            return pResult;
        }

        const CesiumGeospatial::GlobeRectangle& rectangle = pRegion->getRectangle();
        double west = rectangle.getWest();
        double south = rectangle.getSouth();
        double east = rectangle.getEast();
        double north = rectangle.getNorth();

		gsl::span<const uint8_t> encodedIndicesBuffer;
		uint32_t indicesCount = 0;
		size_t indexSizeBytes = 0;
		if (pHeader->vertexCount > 65536) {
			// 32-bit indices
			if ((readIndex % 4) != 0) {
				readIndex += 2;
				if (readIndex > data.size()) {
					return pResult;
				}
			}

			uint32_t triangleCount = readValue<uint32_t>(data, readIndex, 0);
			readIndex += sizeof(uint32_t);
			if (readIndex > data.size()) {
				return pResult;
			}

			indicesCount = triangleCount * 3;
			encodedIndicesBuffer = gsl::span<const uint8_t>(data.data() + readIndex, indicesCount * sizeof(uint32_t));
			readIndex += encodedIndicesBuffer.size_bytes();
			if (readIndex > data.size()) {
				return pResult;
			}

			indexSizeBytes = sizeof(uint32_t);
		}
		else {
			// 16-bit indices
			uint32_t triangleCount = readValue<uint32_t>(data, readIndex, 0);
			readIndex += sizeof(uint32_t);
			if (readIndex > data.size()) {
				return pResult;
			}

			indicesCount = triangleCount * 3;
			encodedIndicesBuffer = gsl::span<const uint8_t>(data.data() + readIndex, indicesCount * sizeof(uint16_t));
			readIndex += encodedIndicesBuffer.size_bytes();
			if (readIndex > data.size()) {
				return pResult;
			}

			indexSizeBytes = sizeof(uint16_t);
		}

		// read the edge indices
		const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
		uint32_t westVertexCount = readValue<uint32_t>(data, readIndex, 0);
		readIndex += sizeof(uint32_t);
		gsl::span<const uint8_t> westEdgeIndicesBuffer(data.data() + readIndex, westVertexCount * indexSizeBytes);
		readIndex += westVertexCount * indexSizeBytes;

		uint32_t southVertexCount = readValue<uint32_t>(data, readIndex, 0);
		readIndex += sizeof(uint32_t);
		gsl::span<const uint8_t> southEdgeIndicesBuffer(data.data() + readIndex, southVertexCount * indexSizeBytes);
		readIndex += southVertexCount * indexSizeBytes;

		uint32_t eastVertexCount = readValue<uint32_t>(data, readIndex, 0);
		readIndex += sizeof(uint32_t);
		gsl::span<const uint8_t> eastEdgeIndicesBuffer(data.data() + readIndex, eastVertexCount * indexSizeBytes);
		readIndex += eastVertexCount * indexSizeBytes;

		uint32_t northVertexCount = readValue<uint32_t>(data, readIndex, 0);
		readIndex += sizeof(uint32_t);
		gsl::span<const uint8_t> northEdgeIndicesBuffer(data.data() + readIndex, northVertexCount * indexSizeBytes);
		readIndex += northVertexCount * indexSizeBytes;

		// estimate skirt size to batch with tile existing indices, vertices, and normals
		size_t skirtIndicesCount = (westVertexCount - 1) * 6 + 
								   (southVertexCount - 1) * 6 + 
								   (eastVertexCount - 1) * 6 + 
								   (northVertexCount - 1) * 6;
		size_t skirtVertexCount = westVertexCount + southVertexCount + eastVertexCount + northVertexCount;

		// decode position without skirt, but preallocate position buffer to include skirt as well
		std::vector<unsigned char> outputPositionsBuffer((vertexCount + skirtVertexCount) * 3 * sizeof(float));
		gsl::span<float> outputPositions(reinterpret_cast<float*>(outputPositionsBuffer.data()), (vertexCount + skirtIndicesCount) * 3);
		size_t positionOutputIndex = 0;

		double minX = std::numeric_limits<double>::max();
		double minY = std::numeric_limits<double>::max();
		double minZ = std::numeric_limits<double>::max();
		double minLatitude = std::numeric_limits<double>::max();
		double minLongitude = std::numeric_limits<double>::max();
		double maxX = std::numeric_limits<double>::lowest();
		double maxY = std::numeric_limits<double>::lowest();
		double maxZ = std::numeric_limits<double>::lowest();
		double maxLatitude = std::numeric_limits<double>::lowest();
		double maxLongitude = std::numeric_limits<double>::lowest();

        int32_t u = 0;
        int32_t v = 0;
        int32_t height = 0;
		std::vector<glm::dvec3> uvsAndHeights;
		uvsAndHeights.reserve(vertexCount);
		for (size_t i = 0; i < vertexCount; ++i) {
			u += zigZagDecode(uBuffer[i]);
			v += zigZagDecode(vBuffer[i]);
			height += zigZagDecode(heightBuffer[i]);

			double uRatio = static_cast<double>(u) / 32767.0;
			double vRatio = static_cast<double>(v) / 32767.0;
			double heightRatio = static_cast<double>(height) / 32767.0;

			double longitude = Math::lerp(west, east, uRatio);
			double latitude = Math::lerp(south, north, vRatio);
			double heightMeters = Math::lerp(minimumHeight, maximumHeight, heightRatio);

			glm::dvec3 position = ellipsoid.cartographicToCartesian(Cartographic(longitude, latitude, heightMeters));
			position -= center;
			outputPositions[positionOutputIndex++] = static_cast<float>(position.x);
			outputPositions[positionOutputIndex++] = static_cast<float>(position.y);
			outputPositions[positionOutputIndex++] = static_cast<float>(position.z);

			minX = glm::min(minX, position.x);
			minY = glm::min(minY, position.y);
			minZ = glm::min(minZ, position.z);
			minLatitude = glm::min(latitude, minLatitude);
			minLongitude = glm::min(longitude, minLongitude);

			maxX = glm::max(maxX, position.x);
			maxY = glm::max(maxY, position.y);
			maxZ = glm::max(maxZ, position.z);
			maxLatitude = glm::max(latitude, maxLatitude);
			maxLongitude = glm::max(longitude, maxLongitude);

			uvsAndHeights.emplace_back(uRatio, vRatio, heightRatio);
		}

		// decode normal vertices of the tile as well as its metadata without skirt
		if (readIndex > data.size()) {
			return pResult;
		}

		std::vector<unsigned char> outputNormalsBuffer;
		while (readIndex < data.size()) {
			if (readIndex + extensionHeaderLength > data.size()) {
				break;
			}

			uint8_t extensionID = *reinterpret_cast<const uint8_t*>(data.data() + readIndex);
			readIndex += sizeof(uint8_t);
			uint32_t extensionLength = *reinterpret_cast<const uint32_t*>(data.data() + readIndex);
			readIndex += sizeof(uint32_t);

			if (extensionID == 1) {
				// Oct-encoded per-vertex normals
				if (readIndex + vertexCount * 2 > data.size()) {
					break;
				}

				const uint8_t* pNormalData = reinterpret_cast<const uint8_t*>(data.data() + readIndex);

				outputNormalsBuffer.resize((vertexCount + skirtVertexCount) * 3 * sizeof(float));
				float* pNormals = reinterpret_cast<float*>(outputNormalsBuffer.data());
				size_t normalOutputIndex = 0;

				for (size_t i = 0; i < vertexCount * 2; i += 2) {
					glm::dvec3 normal = octDecode(pNormalData[i], pNormalData[i + 1]);
					pNormals[normalOutputIndex++] = static_cast<float>(normal.x);
					pNormals[normalOutputIndex++] = static_cast<float>(normal.y);
					pNormals[normalOutputIndex++] = static_cast<float>(normal.z);
				}
			} else if (extensionID == 4) {
				// Metadata
				if (readIndex + sizeof(uint32_t) > data.size()) {
					break;
				}

				uint32_t jsonLength = *reinterpret_cast<const uint32_t*>(data.data() + readIndex);

				if (readIndex + sizeof(uint32_t) + jsonLength > data.size()) {
					break;
				}

				gsl::span<const char> json(reinterpret_cast<const char*>(data.data() + sizeof(uint32_t) + readIndex), jsonLength);
				processMetadata(id, json, *pResult);
			}

			readIndex += extensionLength;
		}
		gsl::span<float> outputNormals(reinterpret_cast<float *>(outputNormalsBuffer.data()), outputNormalsBuffer.size() / sizeof(float));

		// indices buffer for gltf to include tile and skirt indices. Caution of indices type 
		// since adding skirt means the number of vertices is potentially over maximum of uint16_t
		std::vector<unsigned char> outputIndicesBuffer;
		const std::optional<ImplicitTilingContext>& implicitContext = context.implicitContext;
		const QuadtreeTilingScheme& tilingScheme = implicitContext->tilingScheme;
		double skirtHeight = calculateSkirtHeight(id.level, ellipsoid, tilingScheme);
		double longitudeOffset = (maxLongitude - minLongitude) * 0.0001;
		double latitudeOffset = (maxLatitude - minLatitude) * 0.0001;
		glm::vec3 tileNormal = static_cast<glm::vec3>(ellipsoid.geodeticSurfaceNormal(center));
		if (indexSizeBytes == sizeof(uint32_t)) {
			// decode the tile indices without skirt. 
			size_t outputIndicesCount = indicesCount + skirtIndicesCount;
			std::vector<unsigned char> decodedOutputIndicesBuffer(outputIndicesCount * sizeof(uint32_t));
			gsl::span<const uint32_t> indices(reinterpret_cast<const uint32_t *>(encodedIndicesBuffer.data()), indicesCount);
			gsl::span<uint32_t> outputIndices(reinterpret_cast<uint32_t*>(decodedOutputIndicesBuffer.data()), outputIndicesCount);
			decodeIndices(indices, outputIndices);

			addSkirts<uint32_t, uint32_t>(ellipsoid, 
				center, 
				rectangle,
				minimumHeight,
				maximumHeight,
				vertexCount, 
				indicesCount, 
				skirtHeight, 
				longitudeOffset,
				latitudeOffset,
				uvsAndHeights,
				westEdgeIndicesBuffer, 
				southEdgeIndicesBuffer, 
				eastEdgeIndicesBuffer, 
				northEdgeIndicesBuffer, 
				outputPositions, 
				outputNormals, 
				outputIndices);

			outputIndicesBuffer = std::move(decodedOutputIndicesBuffer);
		}
		else {
			// decode the tile indices without skirt. 
			size_t outputIndicesCount = indicesCount + skirtIndicesCount;
			gsl::span<const uint16_t> indices(reinterpret_cast<const uint16_t *>(encodedIndicesBuffer.data()), indicesCount);
			if (vertexCount + skirtVertexCount < std::numeric_limits<uint16_t>::max()) {
				std::vector<unsigned char> decodedOutputIndicesBuffer(outputIndicesCount * sizeof(uint16_t));
				gsl::span<uint16_t> outputIndices(reinterpret_cast<uint16_t*>(decodedOutputIndicesBuffer.data()), outputIndicesCount);
				decodeIndices(indices, outputIndices);

				addSkirts<uint16_t, uint16_t>(ellipsoid, 
					center,
					rectangle,
					minimumHeight,
					maximumHeight,
					vertexCount, 
					indicesCount, 
					skirtHeight, 
					longitudeOffset,
					latitudeOffset,
					uvsAndHeights,
					westEdgeIndicesBuffer, 
					southEdgeIndicesBuffer, 
					eastEdgeIndicesBuffer, 
					northEdgeIndicesBuffer, 
					outputPositions, 
					outputNormals, 
					outputIndices);

				outputIndicesBuffer = std::move(decodedOutputIndicesBuffer);
				indexSizeBytes = sizeof(uint16_t);
			}
			else {
				std::vector<unsigned char> decodedOutputIndicesBuffer(outputIndicesCount * sizeof(uint32_t));
				gsl::span<uint32_t> outputIndices(reinterpret_cast<uint32_t*>(decodedOutputIndicesBuffer.data()), outputIndicesCount);
				decodeIndices(indices, outputIndices);

				addSkirts<uint16_t, uint32_t>(ellipsoid,
					center, 
					rectangle,
					minimumHeight,
					maximumHeight,
					vertexCount, 
					indicesCount, 
					skirtHeight,
					longitudeOffset,
					latitudeOffset,
					uvsAndHeights,
					westEdgeIndicesBuffer, 
					southEdgeIndicesBuffer, 
					eastEdgeIndicesBuffer, 
					northEdgeIndicesBuffer, 
					outputPositions, 
					outputNormals, 
					outputIndices);

				outputIndicesBuffer = std::move(decodedOutputIndicesBuffer);
				indexSizeBytes = sizeof(uint32_t);
			}
		}

		// create gltf
		pResult->model.emplace();
		tinygltf::Model& model = pResult->model.value();
		
		int meshId = static_cast<int>(model.meshes.size());
		model.meshes.emplace_back();
		tinygltf::Mesh& mesh = model.meshes[meshId];
		mesh.primitives.emplace_back();

		tinygltf::Primitive& primitive = mesh.primitives[0];
		primitive.mode = TINYGLTF_MODE_TRIANGLES;
		primitive.material = 0;

		// add position buffer to gltf
		int positionBufferId = static_cast<int>(model.buffers.size());
		model.buffers.emplace_back();
		tinygltf::Buffer& positionBuffer = model.buffers[positionBufferId];
		positionBuffer.data = std::move(outputPositionsBuffer);

		int positionBufferViewId = static_cast<int>(model.bufferViews.size());
		model.bufferViews.emplace_back();
		tinygltf::BufferView& positionBufferView = model.bufferViews[positionBufferViewId];
		positionBufferView.buffer = positionBufferId;
		positionBufferView.byteOffset = 0;
		positionBufferView.byteStride = 3 * sizeof(float);
		positionBufferView.byteLength = positionBuffer.data.size();
		positionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

		int positionAccessorId = static_cast<int>(model.accessors.size());
		model.accessors.emplace_back();
		tinygltf::Accessor& positionAccessor = model.accessors[positionAccessorId];
		positionAccessor.bufferView = positionBufferViewId;
		positionAccessor.byteOffset = 0;
		positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		positionAccessor.count = vertexCount + skirtVertexCount;
		positionAccessor.type = TINYGLTF_TYPE_VEC3;
		positionAccessor.minValues = { minX, minY, minZ };
		positionAccessor.maxValues = { maxX, maxY, maxZ };

		primitive.attributes.emplace("POSITION", positionAccessorId);

		// add normal buffer to gltf if there are any
		if (!outputNormalsBuffer.empty()) {
			int normalBufferId = static_cast<int>(model.buffers.size());
			model.buffers.emplace_back();
			tinygltf::Buffer& normalBuffer = model.buffers[normalBufferId];
			normalBuffer.data = std::move(outputNormalsBuffer);

			int normalBufferViewId = static_cast<int>(model.bufferViews.size());
			model.bufferViews.emplace_back();
			tinygltf::BufferView& normalBufferView = model.bufferViews[normalBufferViewId];
			normalBufferView.buffer = normalBufferId;
			normalBufferView.byteOffset = 0;
			normalBufferView.byteStride = 3 * sizeof(float);
			normalBufferView.byteLength = normalBuffer.data.size();
			normalBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

			int normalAccessorId = static_cast<int>(model.accessors.size());
			model.accessors.emplace_back();
			tinygltf::Accessor& normalAccessor = model.accessors[normalAccessorId];
			normalAccessor.bufferView = normalBufferViewId;
			normalAccessor.byteOffset = 0;
			normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			normalAccessor.count = vertexCount + skirtVertexCount;
			normalAccessor.type = TINYGLTF_TYPE_VEC3;
			
			primitive.attributes.emplace("NORMAL", normalAccessorId);
		}

		// add indices buffer to gltf
		int indicesBufferId = static_cast<int>(model.buffers.size());
		model.buffers.emplace_back();
		tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferId];
		indicesBuffer.data = std::move(outputIndicesBuffer);
		
		int indicesBufferViewId = static_cast<int>(model.bufferViews.size());
		model.bufferViews.emplace_back();
		tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesBufferViewId];
		indicesBufferView.buffer = indicesBufferId;
		indicesBufferView.byteOffset = 0;
		indicesBufferView.byteLength = indicesBuffer.data.size();
		indicesBufferView.byteStride = indexSizeBytes;
		indicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

		int indicesAccessorId = static_cast<int>(model.accessors.size());
		model.accessors.emplace_back();
		tinygltf::Accessor& indicesAccessor = model.accessors[indicesAccessorId];
		indicesAccessor.bufferView = indicesBufferViewId;
		indicesAccessor.byteOffset = 0;
		indicesAccessor.type = TINYGLTF_TYPE_SCALAR;
		indicesAccessor.count = indicesCount + skirtIndicesCount;
		indicesAccessor.componentType = indexSizeBytes == sizeof(uint32_t) ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT : TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;

		primitive.indices = indicesBufferId;

		// create node and update bounding volume
		model.nodes.emplace_back();
		tinygltf::Node& node = model.nodes[0];
		node.mesh = 0;
		node.matrix = {
			1.0, 0.0,  0.0, 0.0,
			0.0, 0.0, -1.0, 0.0,
			0.0, 1.0,  0.0, 0.0,
			center.x, center.z, -center.y, 1.0
		};

		pResult->updatedBoundingVolume = BoundingRegion(rectangle, minimumHeight, maximumHeight);

        return pResult;
    }

    struct TileRange {
        uint32_t minimumX;
        uint32_t minimumY;
        uint32_t maximumX;
        uint32_t maximumY;
    };

    static void from_json(const nlohmann::json& json, TileRange& range) {
        json.at("startX").get_to(range.minimumX);
        json.at("startY").get_to(range.minimumY);
        json.at("endX").get_to(range.maximumX);
        json.at("endY").get_to(range.maximumY);
    }

    static void processMetadata(const QuadtreeTileID& tileID, gsl::span<const char> metadataString, TileContentLoadResult& result) {
        using namespace nlohmann;
        json metadata = json::parse(metadataString.begin(), metadataString.end());

        json::iterator availableIt = metadata.find("available");
        if (availableIt == metadata.end()) {
            return;
        }

        json& available = *availableIt;
        if (available.size() == 0) {
            return;
        }

        uint32_t level = tileID.level + 1;
        for (size_t i = 0; i < available.size(); ++i) {
            std::vector<TileRange> rangesAtLevel = available[i].get<std::vector<TileRange>>();

            for (const TileRange& range : rangesAtLevel) {
                result.availableTileRectangles.push_back(CesiumGeometry::QuadtreeTileRectangularRange {
                    level,
                    range.minimumX,
                    range.minimumY,
                    range.maximumX,
                    range.maximumY
                });
            }

            ++level;
        }
    }

}
