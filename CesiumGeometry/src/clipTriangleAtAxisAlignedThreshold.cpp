#include <CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h>

#include <vector>

namespace CesiumGeometry {
void clipTriangleAtAxisAlignedThreshold(
    double
        threshold,  // Threshold coordinate value at which to clip the triangle
    bool keepAbove, // If true, keep portion of the triangle above threhold;
                    // else keep it below
    int i0,
    int i1,
    int i2,
    double u0, // Coordinate of first vertex in triangle (CCW order)
    double u1, // Coordinate of second vertex in triangle (CCW order)
    double u2, // Coordinate of third vertex in triangle (CCW order)
    std::vector<TriangleClipVertex>&
        result // The aray into which to copy the result.
    ) noexcept {
  bool u0Behind, u1Behind, u2Behind;
  if (keepAbove) {
    u0Behind = u0 < threshold;
    u1Behind = u1 < threshold;
    u2Behind = u2 < threshold;
  } else {
    u0Behind = u0 > threshold;
    u1Behind = u1 > threshold;
    u2Behind = u2 > threshold;
  }

  int numberOfBehind = 0;
  numberOfBehind += u0Behind ? 1 : 0;
  numberOfBehind += u1Behind ? 1 : 0;
  numberOfBehind += u2Behind ? 1 : 0;

  double u01Ratio = 0.0;
  double u02Ratio = 0.0;
  double u12Ratio = 0.0;
  double u10Ratio = 0.0;
  double u20Ratio = 0.0;
  double u21Ratio = 0.0;
  if (numberOfBehind == 1) {
    if (u0Behind) {
      u01Ratio = (threshold - u0) / (u1 - u0);
      u02Ratio = (threshold - u0) / (u2 - u0);
      result.emplace_back(i1);
      result.emplace_back(i2);
      if (u02Ratio != 1.0) {
        result.emplace_back(InterpolatedVertex{i0, i2, u02Ratio});
      }
      if (u01Ratio != 1.0) {
        result.emplace_back(InterpolatedVertex{i0, i1, u01Ratio});
      }
    } else if (u1Behind) {
      u12Ratio = (threshold - u1) / (u2 - u1);
      u10Ratio = (threshold - u1) / (u0 - u1);
      result.emplace_back(i2);
      result.emplace_back(i0);
      if (u10Ratio != 1.0) {
        result.emplace_back(InterpolatedVertex{i1, i0, u10Ratio});
      }
      if (u12Ratio != 1.0) {
        result.emplace_back(InterpolatedVertex{i1, i2, u12Ratio});
      }
    } else if (u2Behind) {
      u20Ratio = (threshold - u2) / (u0 - u2);
      u21Ratio = (threshold - u2) / (u1 - u2);
      result.emplace_back(i0);
      result.emplace_back(i1);
      if (u21Ratio != 1.0) {
        result.emplace_back(InterpolatedVertex{i2, i1, u21Ratio});
      }
      if (u20Ratio != 1.0) {
        result.emplace_back(InterpolatedVertex{i2, i0, u20Ratio});
      }
    }
  } else if (numberOfBehind == 2) {
    if (!u0Behind && u0 != threshold) {
      u10Ratio = (threshold - u1) / (u0 - u1);
      u20Ratio = (threshold - u2) / (u0 - u2);
      result.emplace_back(i0);
      result.emplace_back(InterpolatedVertex{i1, i0, u10Ratio});
      result.emplace_back(InterpolatedVertex{i2, i0, u20Ratio});
    } else if (!u1Behind && u1 != threshold) {
      u21Ratio = (threshold - u2) / (u1 - u2);
      u01Ratio = (threshold - u0) / (u1 - u0);
      result.emplace_back(i1);
      result.emplace_back(InterpolatedVertex{i2, i1, u21Ratio});
      result.emplace_back(InterpolatedVertex{i0, i1, u01Ratio});
    } else if (!u2Behind && u2 != threshold) {
      u02Ratio = (threshold - u0) / (u2 - u0);
      u12Ratio = (threshold - u1) / (u2 - u1);
      result.emplace_back(i2);
      result.emplace_back(InterpolatedVertex{i0, i2, u02Ratio});
      result.emplace_back(InterpolatedVertex{i1, i2, u12Ratio});
    }
  } else if (numberOfBehind != 3) {
    // Completely in front of threshold
    result.emplace_back(i0);
    result.emplace_back(i1);
    result.emplace_back(i2);
  }
  // else completely behind threshold
}

} // namespace CesiumGeometry
