#pragma once
#include "Cesium3DTilesSelection/ViewState.h"
#include "Cesium3DTilesSelection/ViewUpdateResult.h"

namespace Cesium3DTilesSelection {
class DynamicSSE {
public:
  void updateScale(bool overLimit, const ViewState& frustum);
  float getScale() const;

private:
  uint32_t _numFramesConsecutiveNoChange;
  const uint32_t _maxNumFramesConsecutiveNoChange = 4;

  glm::dvec3 _lastDirection;
  glm::dvec3 _lastPosition;

  float scale = 1.0f;
};
} // namespace Cesium3DTilesSelection
