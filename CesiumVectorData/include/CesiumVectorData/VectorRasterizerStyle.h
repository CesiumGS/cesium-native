#pragma once

#include "Color.h"

namespace CesiumVectorData {
  enum class VectorLineWidthMode { Pixels, Meters };
  
  struct VectorRasterizerStyle {
    Color color;
    double lineWidth = 1.0;
    VectorLineWidthMode lineWidthMode = VectorLineWidthMode::Pixels;
  };
}