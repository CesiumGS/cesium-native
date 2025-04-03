#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumVectorData/VectorNode.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumVectorData {
std::string vectorPrimitiveTypeToString(VectorPrimitiveType type) {
  switch (type) {
  case VectorPrimitiveType::Point:
    return "Point";
  case VectorPrimitiveType::MultiPoint:
    return "MultiPoint";
  case VectorPrimitiveType::LineString:
    return "LineString";
  case VectorPrimitiveType::MultiLineString:
    return "MultiLineString";
  case VectorPrimitiveType::Polygon:
    return "Polygon";
  case VectorPrimitiveType::MultiPolygon:
    return "MultiPolygon";
  case VectorPrimitiveType::GeometryCollection:
    return "GeometryCollection";
  case VectorPrimitiveType::Feature:
    return "Feature";
  case VectorPrimitiveType::FeatureCollection:
    return "FeatureCollection";
  }

  throw new std::invalid_argument("Unknown VectorPrimitiveType value");
}

namespace {
template <typename T>
bool vectorEquals(const std::vector<T>& lhs, const std::vector<T>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.size(); i++) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

template <typename T>
bool vectorEquals2D(
    const std::vector<std::vector<T>>& lhs,
    const std::vector<std::vector<T>>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.size(); i++) {
    if (!vectorEquals(lhs[i], rhs[i])) {
      return false;
    }
  }

  return true;
}

template <typename T>
bool vectorEquals3D(
    const std::vector<std::vector<std::vector<T>>>& lhs,
    const std::vector<std::vector<std::vector<T>>>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.size(); i++) {
    if (!vectorEquals2D(lhs[i], rhs[i])) {
      return false;
    }
  }

  return true;
}

bool boundingBoxEquals(
    const std::optional<BoundingRegion>& lhs,
    const std::optional<BoundingRegion>& rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return false;
  }

  return !lhs.has_value() || BoundingRegion::equals(*lhs, *rhs);
}
} // namespace

bool operator==(const VectorNode& lhs, const VectorNode& rhs) {
  struct VectorNodeEqualsVisitor {
    const VectorNode& rhs;
    bool operator()(const Point& lhs) { return lhs == rhs; }
    bool operator()(const MultiPoint& lhs) { return lhs == rhs; }
    bool operator()(const LineString& lhs) { return lhs == rhs; }
    bool operator()(const MultiLineString& lhs) { return lhs == rhs; }
    bool operator()(const Polygon& lhs) { return lhs == rhs; }
    bool operator()(const MultiPolygon& lhs) { return lhs == rhs; }
    bool operator()(const GeometryCollection& lhs) { return lhs == rhs; }
    bool operator()(const Feature& lhs) { return lhs == rhs; }
    bool operator()(const FeatureCollection& lhs) { return lhs == rhs; }
  };

  return std::visit(VectorNodeEqualsVisitor{rhs}, lhs);
}

bool operator==(const GeometryPrimitive& lhs, const GeometryPrimitive& rhs) {
  return geometryPrimitiveToVectorNode(lhs) ==
         geometryPrimitiveToVectorNode(rhs);
}

bool operator==(const Point& lhs, const VectorNode& rhs) {
  const Point* pRhsPoint = std::get_if<Point>(&rhs);
  if (!pRhsPoint) {
    return false;
  }

  if (pRhsPoint->coordinates != lhs.coordinates) {
    return false;
  }

  if (pRhsPoint->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsPoint->boundingBox);
}

bool operator==(const MultiPoint& lhs, const VectorNode& rhs) {
  const MultiPoint* pRhsMultiPoint = std::get_if<MultiPoint>(&rhs);
  if (!pRhsMultiPoint) {
    return false;
  }

  if (!vectorEquals(lhs.coordinates, pRhsMultiPoint->coordinates)) {
    return false;
  }

  if (pRhsMultiPoint->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsMultiPoint->boundingBox);
}

bool operator==(const LineString& lhs, const VectorNode& rhs) {
  const LineString* pRhsLineString = std::get_if<LineString>(&rhs);
  if (!pRhsLineString) {
    return false;
  }

  if (!vectorEquals(lhs.coordinates, pRhsLineString->coordinates)) {
    return false;
  }

  if (pRhsLineString->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsLineString->boundingBox);
}

bool operator==(const MultiLineString& lhs, const VectorNode& rhs) {
  const MultiLineString* pRhsMultiLineString =
      std::get_if<MultiLineString>(&rhs);
  if (!pRhsMultiLineString) {
    return false;
  }

  if (!vectorEquals2D(lhs.coordinates, pRhsMultiLineString->coordinates)) {
    return false;
  }

  if (pRhsMultiLineString->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsMultiLineString->boundingBox);
}

bool operator==(const Polygon& lhs, const VectorNode& rhs) {
  const Polygon* pRhsPolygon = std::get_if<Polygon>(&rhs);
  if (!pRhsPolygon) {
    return false;
  }

  if (!vectorEquals2D(lhs.coordinates, pRhsPolygon->coordinates)) {
    return false;
  }

  if (pRhsPolygon->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsPolygon->boundingBox);
}

bool operator==(const MultiPolygon& lhs, const VectorNode& rhs) {
  const MultiPolygon* pRhsMultiPolygon = std::get_if<MultiPolygon>(&rhs);
  if (!pRhsMultiPolygon) {
    return false;
  }

  if (!vectorEquals3D(lhs.coordinates, pRhsMultiPolygon->coordinates)) {
    return false;
  }

  if (pRhsMultiPolygon->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsMultiPolygon->boundingBox);
}

bool operator==(const GeometryCollection& lhs, const VectorNode& rhs) {
  const GeometryCollection* pRhsGeometryCollection =
      std::get_if<GeometryCollection>(&rhs);
  if (!pRhsGeometryCollection) {
    return false;
  }

  if (!vectorEquals(lhs.geometries, pRhsGeometryCollection->geometries)) {
    return false;
  }

  if (lhs.foreignMembers != pRhsGeometryCollection->foreignMembers) {
    return false;
  }

  return boundingBoxEquals(
      lhs.boundingBox,
      pRhsGeometryCollection->boundingBox);
}

bool operator==(const Feature& lhs, const VectorNode& rhs) {
  const Feature* pRhsFeature = std::get_if<Feature>(&rhs);
  if (!pRhsFeature) {
    return false;
  }

  return lhs == *pRhsFeature;
}

bool operator==(const Feature& lhs, const Feature& rhs) {
  struct FeatureIdVisitor {
    const Feature& rhs;
    bool operator()(const std::string& lhs) {
      const std::string* pRhs = std::get_if<std::string>(&rhs.id);
      return pRhs && lhs == *pRhs;
    }
    bool operator()(const int64_t& lhs) {
      const int64_t* pRhs = std::get_if<int64_t>(&rhs.id);
      return pRhs && lhs == *pRhs;
    }
    bool operator()(const std::monostate&) {
      const std::monostate* pRhs = std::get_if<std::monostate>(&rhs.id);
      return pRhs;
    }
  };

  if (!std::visit(FeatureIdVisitor{rhs}, lhs.id)) {
    return false;
  }

  if (lhs.geometry != rhs.geometry) {
    return false;
  }

  if (lhs.properties != rhs.properties) {
    return false;
  }

  if (lhs.foreignMembers != rhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, rhs.boundingBox);
}

bool operator==(const FeatureCollection& lhs, const VectorNode& rhs) {
  const FeatureCollection* pRhsFeatureCollection =
      std::get_if<FeatureCollection>(&rhs);
  if (!pRhsFeatureCollection) {
    return false;
  }

  if (!vectorEquals(lhs.features, pRhsFeatureCollection->features)) {
    return false;
  }

  if (pRhsFeatureCollection->foreignMembers != lhs.foreignMembers) {
    return false;
  }

  return boundingBoxEquals(lhs.boundingBox, pRhsFeatureCollection->boundingBox);
}

VectorNode geometryPrimitiveToVectorNode(const GeometryPrimitive& geometry) {
  struct GeometryPrimitiveToVectorNodeVisitor {
    VectorNode operator()(const Point& lhs) { return lhs; }
    VectorNode operator()(const MultiPoint& lhs) { return lhs; }
    VectorNode operator()(const LineString& lhs) { return lhs; }
    VectorNode operator()(const MultiLineString& lhs) { return lhs; }
    VectorNode operator()(const Polygon& lhs) { return lhs; }
    VectorNode operator()(const MultiPolygon& lhs) { return lhs; }
    VectorNode operator()(const GeometryCollection& lhs) { return lhs; }
  };

  return std::visit(GeometryPrimitiveToVectorNodeVisitor{}, geometry);
}
} // namespace CesiumVectorData