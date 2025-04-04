#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/CompositeCartographicPolygon.h"

#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumVectorData/VectorNode.h>

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumVectorData {

namespace {
struct GeometryEqualVisitor {
  const VectorPrimitive& rhs;
  bool operator()(const Cartographic& lhs) {
    const Cartographic* pRhs = std::get_if<Cartographic>(&rhs);
    if (!pRhs) {
      return false;
    }

    return lhs == *pRhs;
  }

  bool operator()(const std::vector<Cartographic>& lhs) {
    const std::vector<Cartographic>* pRhs =
        std::get_if<std::vector<Cartographic>>(&rhs);
    if (!pRhs) {
      return false;
    }

    if (lhs.size() != pRhs->size()) {
      return false;
    }

    for (size_t i = 0; i < lhs.size(); i++) {
      if (lhs[i] != (*pRhs)[i]) {
        return false;
      }
    }

    return true;
  }

  bool operator()(const CompositeCartographicPolygon& lhs) {
    const CompositeCartographicPolygon* pRhs =
        std::get_if<CompositeCartographicPolygon>(&rhs);
    if (!pRhs) {
      return false;
    }

    return lhs == *pRhs;
  }
};
} // namespace

bool VectorNode::operator==(const VectorNode& rhs) const {
  if (this->id != rhs.id) {
    return false;
  }

  if (this->boundingBox.has_value() != rhs.boundingBox.has_value()) {
    return false;
  }

  if (this->boundingBox &&
      !BoundingRegion::equals(*this->boundingBox, *rhs.boundingBox)) {
    return false;
  }

  if (this->properties != rhs.properties) {
    return false;
  }

  if (this->foreignMembers != rhs.foreignMembers) {
    return false;
  }

  if (this->primitives.size() != rhs.primitives.size()) {
    return false;
  }

  for (size_t i = 0; i < this->primitives.size(); i++) {
    if (!std::visit(
            GeometryEqualVisitor{rhs.primitives[i]},
            this->primitives[i])) {
      return false;
    }
  }

  if (this->children.size() != rhs.children.size()) {
    return false;
  }

  for (size_t i = 0; i < this->children.size(); i++) {
    if (!this->children[i].operator==(rhs.children[i])) {
      return false;
    }
  }

  return true;
}

} // namespace CesiumVectorData