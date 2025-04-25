#pragma once

#include "Color.h"

namespace CesiumVectorData {
enum class VectorLineWidthMode : uint8_t { Pixels = 0, Meters = 1 };

struct VectorRasterizerStyle {
  Color color;
  double lineWidth = 1.0;
  VectorLineWidthMode lineWidthMode = VectorLineWidthMode::Pixels;
};
} // namespace CesiumVectorData