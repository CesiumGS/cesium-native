#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/GeoJsonSpatialIndex.h>
#include <CesiumVectorData/VectorStyle.h>

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumVectorData {

namespace {
// The number of children of each node of the tree.
const size_t NODE_SIZE = 16;

using Ring = std::vector<glm::dvec3>;
using PolygonRings = std::vector<Ring>;

glm::dvec2 toRadians(const glm::dvec3& degrees) {
  return glm::dvec2(
      Math::degreesToRadians(degrees.x),
      Math::degreesToRadians(degrees.y));
}

/**
 * @brief Converts longitude and latitude in radians to meters in a plane
 * tangent to the ellipsoid at a given origin.
 *
 * The conversion is linear in longitude and latitude, so straight edges in
 * longitude and latitude, which is how geometry is drawn, stay straight.
 */
struct LocalPlane {
  double originLongitude;
  double originLatitude;
  double metersPerRadianX;
  double metersPerRadianY;

  LocalPlane(double longitude, double latitude, double radius)
      : originLongitude(longitude),
        originLatitude(latitude),
        metersPerRadianX(radius * std::cos(latitude)),
        metersPerRadianY(radius) {}

  glm::dvec2 project(const glm::dvec3& degrees) const {
    const glm::dvec2 radians = toRadians(degrees);
    return glm::dvec2(
        (radians.x - this->originLongitude) * this->metersPerRadianX,
        (radians.y - this->originLatitude) * this->metersPerRadianY);
  }
};

// Distance from the origin to the segment from a to b.
double distanceToSegment(const glm::dvec2& a, const glm::dvec2& b) {
  const glm::dvec2 ab = b - a;
  const double lengthSquared = glm::dot(ab, ab);
  if (lengthSquared == 0.0) {
    return glm::length(a);
  }
  const double t = std::clamp(-glm::dot(a, ab) / lengthSquared, 0.0, 1.0);
  return glm::length(a + ab * t);
}

double distanceToLine(
    const LocalPlane& plane,
    const std::vector<glm::dvec3>& points,
    bool closed) {
  double result = std::numeric_limits<double>::max();
  if (points.empty()) {
    return result;
  }

  glm::dvec2 previous = plane.project(points[0]);
  if (points.size() == 1) {
    return glm::length(previous);
  }

  for (size_t i = 1; i < points.size(); ++i) {
    const glm::dvec2 current = plane.project(points[i]);
    result = std::min(result, distanceToSegment(previous, current));
    previous = current;
  }

  if (closed) {
    result =
        std::min(result, distanceToSegment(previous, plane.project(points[0])));
  }

  return result;
}

// Even-odd test of whether a point lies inside a ring, in radians.
bool isInsideRing(const glm::dvec2& point, const Ring& ring) {
  bool inside = false;
  const size_t count = ring.size();
  if (count < 3) {
    return false;
  }

  glm::dvec2 previous = toRadians(ring[count - 1]);
  for (size_t i = 0; i < count; ++i) {
    const glm::dvec2 current = toRadians(ring[i]);
    if ((current.y > point.y) != (previous.y > point.y)) {
      const double crossingX = current.x + (point.y - current.y) *
                                               (previous.x - current.x) /
                                               (previous.y - current.y);
      if (point.x < crossingX) {
        inside = !inside;
      }
    }
    previous = current;
  }

  return inside;
}

bool isInsidePolygon(const glm::dvec2& point, const PolygonRings& polygon) {
  if (polygon.empty() || !isInsideRing(point, polygon[0])) {
    return false;
  }

  for (size_t i = 1; i < polygon.size(); ++i) {
    if (isInsideRing(point, polygon[i])) {
      return false;
    }
  }

  return true;
}

struct PlanarRectangle {
  double west;
  double south;
  double east;
  double north;

  bool contains(const glm::dvec2& point) const {
    return point.x >= this->west && point.x <= this->east &&
           point.y >= this->south && point.y <= this->north;
  }

  // Liang-Barsky clipping of the segment from a to b against this rectangle.
  bool intersectsSegment(const glm::dvec2& a, const glm::dvec2& b) const {
    const glm::dvec2 d = b - a;
    double t0 = 0.0;
    double t1 = 1.0;
    const double p[4] = {-d.x, d.x, -d.y, d.y};
    const double q[4] = {
        a.x - this->west,
        this->east - a.x,
        a.y - this->south,
        this->north - a.y};
    for (size_t i = 0; i < 4; ++i) {
      if (p[i] == 0.0) {
        if (q[i] < 0.0) {
          return false;
        }
      } else {
        const double t = q[i] / p[i];
        if (p[i] < 0.0) {
          t0 = std::max(t0, t);
        } else {
          t1 = std::min(t1, t);
        }
        if (t0 > t1) {
          return false;
        }
      }
    }
    return true;
  }

  bool
  intersectsLine(const std::vector<glm::dvec3>& points, bool closed) const {
    if (points.empty()) {
      return false;
    }

    glm::dvec2 previous = toRadians(points[0]);
    if (this->contains(previous)) {
      return true;
    }

    for (size_t i = 1; i < points.size(); ++i) {
      const glm::dvec2 current = toRadians(points[i]);
      if (this->intersectsSegment(previous, current)) {
        return true;
      }
      previous = current;
    }

    return closed && points.size() > 2 &&
           this->intersectsSegment(previous, toRadians(points[0]));
  }

  bool intersectsPolygon(const PolygonRings& polygon) const {
    for (const Ring& ring : polygon) {
      if (this->intersectsLine(ring, true)) {
        return true;
      }
    }

    // No edge crosses the rectangle, so the rectangle is either entirely
    // inside the polygon or entirely outside it.
    return isInsidePolygon(
        glm::dvec2(
            (this->west + this->east) * 0.5,
            (this->south + this->north) * 0.5),
        polygon);
  }
};

void expandBox(
    std::optional<glm::dvec4>& box,
    const std::vector<glm::dvec3>& points) {
  for (const glm::dvec3& point : points) {
    const glm::dvec2 radians = toRadians(point);
    if (!box) {
      box = glm::dvec4(radians.x, radians.y, radians.x, radians.y);
    } else {
      box->x = std::min(box->x, radians.x);
      box->y = std::min(box->y, radians.y);
      box->z = std::max(box->z, radians.x);
      box->w = std::max(box->w, radians.y);
    }
  }
}

void addLineWidth(
    const LineStyle& style,
    double& halfWidthMeters,
    double& halfWidthPixels) {
  if (style.widthMode == LineWidthMode::Meters) {
    halfWidthMeters = std::max(halfWidthMeters, style.width * 0.5);
  } else {
    halfWidthPixels = std::max(halfWidthPixels, style.width * 0.5);
  }
}
} // namespace

GeoJsonSpatialIndex::GeoJsonSpatialIndex(
    const std::shared_ptr<const GeoJsonDocument>& pDocument,
    const VectorStyle& defaultStyle,
    const Ellipsoid& ellipsoid)
    : _pDocument(pDocument), _radius(ellipsoid.getRadii().x) {
  if (this->_pDocument) {
    this->addObject(this->_pDocument->rootObject, defaultStyle, nullptr);
  }
  this->buildTree();
}

void GeoJsonSpatialIndex::addObject(
    const GeoJsonObject& object,
    const VectorStyle& parentStyle,
    const GeoJsonObject* pFeature) {
  const std::optional<VectorStyle>& ownStyle = object.getStyle();
  const VectorStyle& style = ownStyle ? *ownStyle : parentStyle;

  if (const GeoJsonFeature* pFeatureValue = object.getIf<GeoJsonFeature>()) {
    if (pFeatureValue->geometry) {
      this->addObject(*pFeatureValue->geometry, style, &object);
    }
  } else if (
      const GeoJsonFeatureCollection* pFeatures =
          object.getIf<GeoJsonFeatureCollection>()) {
    for (const GeoJsonObject& child : pFeatures->features) {
      this->addObject(child, style, pFeature);
    }
  } else if (
      const GeoJsonGeometryCollection* pGeometries =
          object.getIf<GeoJsonGeometryCollection>()) {
    for (const GeoJsonObject& child : pGeometries->geometries) {
      this->addObject(child, style, pFeature);
    }
  } else {
    this->addGeometry(object, style, pFeature);
  }
}

void GeoJsonSpatialIndex::addGeometry(
    const GeoJsonObject& object,
    const VectorStyle& style,
    const GeoJsonObject* pFeature) {
  std::optional<glm::dvec4> box;
  Kind kind = Kind::Point;
  double halfWidthMeters = 0.0;
  double halfWidthPixels = 0.0;

  if (const GeoJsonPoint* pPoint = object.getIf<GeoJsonPoint>()) {
    expandBox(box, {pPoint->coordinates});
  } else if (
      const GeoJsonMultiPoint* pPoints = object.getIf<GeoJsonMultiPoint>()) {
    expandBox(box, pPoints->coordinates);
  } else if (
      const GeoJsonLineString* pLine = object.getIf<GeoJsonLineString>()) {
    kind = Kind::Line;
    expandBox(box, pLine->coordinates);
  } else if (
      const GeoJsonMultiLineString* pLines =
          object.getIf<GeoJsonMultiLineString>()) {
    kind = Kind::Line;
    for (const std::vector<glm::dvec3>& line : pLines->coordinates) {
      expandBox(box, line);
    }
  } else if (const GeoJsonPolygon* pPolygon = object.getIf<GeoJsonPolygon>()) {
    kind = Kind::Polygon;
    for (const Ring& ring : pPolygon->coordinates) {
      expandBox(box, ring);
    }
  } else if (
      const GeoJsonMultiPolygon* pPolygons =
          object.getIf<GeoJsonMultiPolygon>()) {
    kind = Kind::Polygon;
    for (const PolygonRings& polygon : pPolygons->coordinates) {
      for (const Ring& ring : polygon) {
        expandBox(box, ring);
      }
    }
  }

  if (!box) {
    return;
  }

  switch (kind) {
  case Kind::Point:
    // The point's circle is drawn with its radius in pixels, and its outline
    // is centered on the circle's edge.
    halfWidthPixels = style.point.radius;
    if (style.point.outline) {
      if (style.point.outline->widthMode == LineWidthMode::Meters) {
        halfWidthMeters = style.point.outline->width * 0.5;
      } else {
        halfWidthPixels += style.point.outline->width * 0.5;
      }
    }
    break;
  case Kind::Line:
    addLineWidth(style.line, halfWidthMeters, halfWidthPixels);
    break;
  case Kind::Polygon:
    if (style.polygon.outline) {
      addLineWidth(*style.polygon.outline, halfWidthMeters, halfWidthPixels);
    }
    break;
  }

  this->_maxHalfWidthMeters =
      std::max(this->_maxHalfWidthMeters, halfWidthMeters);
  this->_maxHalfWidthPixels =
      std::max(this->_maxHalfWidthPixels, halfWidthPixels);

  this->_entries.emplace_back(Entry{
      &object,
      pFeature,
      kind,
      Box{box->x, box->y, box->z, box->w},
      halfWidthMeters,
      halfWidthPixels});
}

void GeoJsonSpatialIndex::buildTree() {
  // A sort-tile-recursive packed R-tree. Each level groups the boxes of the
  // level below into nodes of up to NODE_SIZE children, sorting by longitude
  // into vertical slices and then by latitude within each slice.
  struct Item {
    Box box;
    uint32_t index;
  };

  std::vector<Item> items;
  items.reserve(this->_entries.size());
  for (size_t i = 0; i < this->_entries.size(); ++i) {
    items.emplace_back(Item{this->_entries[i].box, static_cast<uint32_t>(i)});
  }

  if (items.empty()) {
    return;
  }

  const auto centerX = [](const Item& item) {
    return item.box.west + item.box.east;
  };
  const auto centerY = [](const Item& item) {
    return item.box.south + item.box.north;
  };

  // Nodes are built level by level from the leaves up. `levels` records the
  // range of `nodes` for each level so that they can be stored root first.
  std::vector<std::vector<Node>> levels;
  bool isLeafLevel = true;

  while (isLeafLevel || items.size() > 1) {
    const size_t nodeCount = (items.size() + NODE_SIZE - 1) / NODE_SIZE;
    const size_t sliceCount = static_cast<size_t>(
        std::ceil(std::sqrt(static_cast<double>(nodeCount))));
    const size_t sliceSize = sliceCount * NODE_SIZE;

    std::sort(items.begin(), items.end(), [&](const Item& a, const Item& b) {
      return centerX(a) < centerX(b);
    });
    for (size_t start = 0; start < items.size(); start += sliceSize) {
      const auto sliceEnd =
          items.begin() + static_cast<std::ptrdiff_t>(
                              std::min(start + sliceSize, items.size()));
      std::sort(
          items.begin() + static_cast<std::ptrdiff_t>(start),
          sliceEnd,
          [&](const Item& a, const Item& b) {
            return centerY(a) < centerY(b);
          });
    }

    std::vector<Node> level;
    std::vector<Item> parents;
    level.reserve(nodeCount);
    parents.reserve(nodeCount);
    for (size_t start = 0; start < items.size(); start += NODE_SIZE) {
      const size_t end = std::min(start + NODE_SIZE, items.size());
      Box box = items[start].box;
      for (size_t i = start; i < end; ++i) {
        box.west = std::min(box.west, items[i].box.west);
        box.south = std::min(box.south, items[i].box.south);
        box.east = std::max(box.east, items[i].box.east);
        box.north = std::max(box.north, items[i].box.north);
        if (isLeafLevel) {
          this->_leafEntries.emplace_back(items[i].index);
        }
      }

      // A node's children are `items[start, end)`. For a leaf, that is the
      // same range of `_leafEntries`. Otherwise, it is the same range of the
      // level below once that level is reordered to match `items`, below.
      // Until the levels are stored, `first` is relative to that level.
      parents.emplace_back(Item{box, static_cast<uint32_t>(level.size())});
      level.emplace_back(Node{
          box,
          static_cast<uint32_t>(start),
          static_cast<uint32_t>(end - start),
          isLeafLevel});
    }

    if (!isLeafLevel) {
      // Reorder the level below so that each node's children are contiguous,
      // in the sorted order of `items`.
      std::vector<Node>& below = levels.back();
      std::vector<Node> reordered;
      reordered.reserve(below.size());
      for (const Item& item : items) {
        reordered.emplace_back(below[item.index]);
      }
      below = std::move(reordered);
    }

    levels.emplace_back(std::move(level));
    items = std::move(parents);
    isLeafLevel = false;
  }

  // Store the levels root first, turning each `first` into an index into
  // `_nodes`.
  size_t total = 0;
  for (const std::vector<Node>& level : levels) {
    total += level.size();
  }
  this->_nodes.reserve(total);

  std::vector<size_t> levelOffsets(levels.size());
  size_t offset = 0;
  for (size_t i = levels.size(); i-- > 0;) {
    levelOffsets[i] = offset;
    offset += levels[i].size();
  }

  for (size_t i = levels.size(); i-- > 0;) {
    for (Node node : levels[i]) {
      if (!node.isLeaf) {
        node.first += static_cast<uint32_t>(levelOffsets[i - 1]);
      }
      this->_nodes.emplace_back(node);
    }
  }
}

template <typename Callback>
void GeoJsonSpatialIndex::forEachCandidate(const Box& box, Callback&& callback)
    const {
  if (this->_nodes.empty()) {
    return;
  }

  const auto overlaps = [&box](const Box& other) {
    return other.west <= box.east && other.east >= box.west &&
           other.south <= box.north && other.north >= box.south;
  };

  std::vector<uint32_t> stack{0};
  while (!stack.empty()) {
    const Node& node = this->_nodes[stack.back()];
    stack.pop_back();
    if (!overlaps(node.box)) {
      continue;
    }

    for (uint32_t i = node.first; i < node.first + node.count; ++i) {
      if (node.isLeaf) {
        const uint32_t entryIndex = this->_leafEntries[i];
        if (overlaps(this->_entries[entryIndex].box)) {
          callback(entryIndex);
        }
      } else {
        stack.emplace_back(i);
      }
    }
  }
}

std::vector<GeoJsonPickResult> GeoJsonSpatialIndex::pick(
    const Cartographic& position,
    const GeoJsonPickOptions& options) const {
  std::vector<GeoJsonPickResult> results;
  if (options.maxResults == 0 || this->_entries.empty()) {
    return results;
  }

  const double tolerance = std::max(options.toleranceMeters, 0.0);
  const double metersPerPixel =
      std::max(options.metersPerPixel.value_or(0.0), 0.0);
  const double searchMeters = tolerance + this->_maxHalfWidthMeters +
                              this->_maxHalfWidthPixels * metersPerPixel;

  // The search box in radians. Longitude degrees shrink toward the poles, so
  // use the latitude in the box nearest a pole to keep the box conservative.
  const double latitude = position.latitude;
  const double searchLatitude = searchMeters / this->_radius;
  const double farthestLatitude =
      std::min(std::abs(latitude) + searchLatitude, Math::PiOverTwo);
  const double cosine = std::cos(farthestLatitude);
  const double searchLongitude =
      cosine * this->_radius > searchMeters / Math::OnePi
          ? searchMeters / (this->_radius * cosine)
          : Math::OnePi;

  struct Hit {
    uint32_t entryIndex;
    double distance;
  };
  std::vector<Hit> hits;

  // Geometry near the antimeridian may be on the other side of it from the
  // position, so also search with the position moved by a full turn.
  const double longitudes[3] = {
      position.longitude,
      position.longitude - Math::TwoPi,
      position.longitude + Math::TwoPi};
  for (const double longitude : longitudes) {
    const Box box{
        longitude - searchLongitude,
        latitude - searchLatitude,
        longitude + searchLongitude,
        latitude + searchLatitude};
    if (longitude != position.longitude &&
        (box.east < -Math::OnePi || box.west > Math::OnePi)) {
      continue;
    }

    const LocalPlane plane(longitude, latitude, this->_radius);
    const glm::dvec2 point(longitude, latitude);

    this->forEachCandidate(box, [&](uint32_t entryIndex) {
      const Entry& entry = this->_entries[entryIndex];
      const double reach = tolerance + entry.halfWidthMeters +
                           entry.halfWidthPixels * metersPerPixel;
      double distance = std::numeric_limits<double>::max();

      const GeoJsonObject& object = *entry.pGeometry;
      if (const GeoJsonPoint* pPoint = object.getIf<GeoJsonPoint>()) {
        distance = glm::length(plane.project(pPoint->coordinates));
      } else if (
          const GeoJsonMultiPoint* pPoints =
              object.getIf<GeoJsonMultiPoint>()) {
        for (const glm::dvec3& coordinates : pPoints->coordinates) {
          distance =
              std::min(distance, glm::length(plane.project(coordinates)));
        }
      } else if (
          const GeoJsonLineString* pLine = object.getIf<GeoJsonLineString>()) {
        distance = distanceToLine(plane, pLine->coordinates, false);
      } else if (
          const GeoJsonMultiLineString* pLines =
              object.getIf<GeoJsonMultiLineString>()) {
        for (const std::vector<glm::dvec3>& line : pLines->coordinates) {
          distance = std::min(distance, distanceToLine(plane, line, false));
        }
      } else {
        const auto testPolygon = [&](const PolygonRings& polygon) {
          if (isInsidePolygon(point, polygon)) {
            distance = 0.0;
            return;
          }
          for (const Ring& ring : polygon) {
            distance = std::min(distance, distanceToLine(plane, ring, true));
          }
        };
        if (const GeoJsonPolygon* pPolygon = object.getIf<GeoJsonPolygon>()) {
          testPolygon(pPolygon->coordinates);
        } else if (
            const GeoJsonMultiPolygon* pPolygons =
                object.getIf<GeoJsonMultiPolygon>()) {
          for (const PolygonRings& polygon : pPolygons->coordinates) {
            testPolygon(polygon);
            if (distance == 0.0) {
              break;
            }
          }
        }
      }

      if (distance <= reach) {
        hits.emplace_back(Hit{entryIndex, distance});
      }
    });
  }

  // Keep the nearest hit for each entry, since an entry may be found by more
  // than one of the searches above.
  std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
    return a.entryIndex < b.entryIndex ||
           (a.entryIndex == b.entryIndex && a.distance < b.distance);
  });
  hits.erase(
      std::unique(
          hits.begin(),
          hits.end(),
          [](const Hit& a, const Hit& b) {
            return a.entryIndex == b.entryIndex;
          }),
      hits.end());

  std::stable_sort(hits.begin(), hits.end(), [&](const Hit& a, const Hit& b) {
    const Kind kindA = this->_entries[a.entryIndex].kind;
    const Kind kindB = this->_entries[b.entryIndex].kind;
    if (kindA != kindB) {
      return kindA < kindB;
    }
    return a.distance < b.distance;
  });

  const size_t count = std::min(hits.size(), options.maxResults);
  results.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const Entry& entry = this->_entries[hits[i].entryIndex];
    results.emplace_back(
        GeoJsonPickResult{entry.pGeometry, entry.pFeature, hits[i].distance});
  }

  return results;
}

std::vector<GeoJsonPickResult> GeoJsonSpatialIndex::pick(
    const GlobeRectangle& rectangle,
    size_t maxResults) const {
  std::vector<GeoJsonPickResult> results;
  if (maxResults == 0 || this->_entries.empty()) {
    return results;
  }

  // A rectangle that crosses the antimeridian is searched as two rectangles,
  // one on each side of it.
  std::vector<PlanarRectangle> rectangles;
  if (rectangle.getWest() <= rectangle.getEast()) {
    rectangles.emplace_back(PlanarRectangle{
        rectangle.getWest(),
        rectangle.getSouth(),
        rectangle.getEast(),
        rectangle.getNorth()});
  } else {
    rectangles.emplace_back(PlanarRectangle{
        rectangle.getWest(),
        rectangle.getSouth(),
        Math::OnePi,
        rectangle.getNorth()});
    rectangles.emplace_back(PlanarRectangle{
        -Math::OnePi,
        rectangle.getSouth(),
        rectangle.getEast(),
        rectangle.getNorth()});
  }

  std::vector<uint32_t> found;
  for (const PlanarRectangle& planar : rectangles) {
    const Box box{planar.west, planar.south, planar.east, planar.north};
    this->forEachCandidate(box, [&](uint32_t entryIndex) {
      const GeoJsonObject& object = *this->_entries[entryIndex].pGeometry;
      bool intersects = false;
      if (const GeoJsonPoint* pPoint = object.getIf<GeoJsonPoint>()) {
        intersects = planar.contains(toRadians(pPoint->coordinates));
      } else if (
          const GeoJsonMultiPoint* pPoints =
              object.getIf<GeoJsonMultiPoint>()) {
        intersects = std::any_of(
            pPoints->coordinates.begin(),
            pPoints->coordinates.end(),
            [&](const glm::dvec3& coordinates) {
              return planar.contains(toRadians(coordinates));
            });
      } else if (
          const GeoJsonLineString* pLine = object.getIf<GeoJsonLineString>()) {
        intersects = planar.intersectsLine(pLine->coordinates, false);
      } else if (
          const GeoJsonMultiLineString* pLines =
              object.getIf<GeoJsonMultiLineString>()) {
        intersects = std::any_of(
            pLines->coordinates.begin(),
            pLines->coordinates.end(),
            [&](const std::vector<glm::dvec3>& line) {
              return planar.intersectsLine(line, false);
            });
      } else if (
          const GeoJsonPolygon* pPolygon = object.getIf<GeoJsonPolygon>()) {
        intersects = planar.intersectsPolygon(pPolygon->coordinates);
      } else if (
          const GeoJsonMultiPolygon* pPolygons =
              object.getIf<GeoJsonMultiPolygon>()) {
        intersects = std::any_of(
            pPolygons->coordinates.begin(),
            pPolygons->coordinates.end(),
            [&](const PolygonRings& polygon) {
              return planar.intersectsPolygon(polygon);
            });
      }

      if (intersects) {
        found.emplace_back(entryIndex);
      }
    });
  }

  // Entries are stored in document order.
  std::sort(found.begin(), found.end());
  found.erase(std::unique(found.begin(), found.end()), found.end());

  const size_t count = std::min(found.size(), maxResults);
  results.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const Entry& entry = this->_entries[found[i]];
    results.emplace_back(
        GeoJsonPickResult{entry.pGeometry, entry.pFeature, 0.0});
  }

  return results;
}

} // namespace CesiumVectorData
