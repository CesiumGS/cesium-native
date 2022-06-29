#include "Cesium3DTilesSelection/DynamicSSE.h"

void Cesium3DTilesSelection::DynamicSSE::updateScale(
    bool overLimit,
    const ViewState& frustum) {
  if (_lastPosition != frustum.getPosition() &&
      _lastDirection != frustum.getDirection()) {
    _lastPosition = frustum.getPosition();
    _lastDirection = frustum.getDirection();
    scale = 1.0f;
  } else if (
      ++_numFramesConsecutiveNoChange > _maxNumFramesConsecutiveNoChange &&
      overLimit) {
    scale += .1f;
  }
}

float Cesium3DTilesSelection::DynamicSSE::getScale() const { return scale; }
