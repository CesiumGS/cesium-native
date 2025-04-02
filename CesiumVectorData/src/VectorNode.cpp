#include <CesiumVectorData/VectorNode.h>

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
}
} // namespace CesiumVectorData