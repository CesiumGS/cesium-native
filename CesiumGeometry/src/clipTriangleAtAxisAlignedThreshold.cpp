#include "CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h"

namespace CesiumGeometry
{
    TriangleClipVertex::TriangleClipVertex(int index_) :
        index(index_),
        first(-1),
        second(-1),
        t(0.0)
    {
    }

    TriangleClipVertex::TriangleClipVertex(int first_, int second_, double t_) :
        index(-1),
        first(first_),
        second(second_),
        t(t_)
    {
    }

    void clipTriangleAtAxisAlignedThreshold(
        double threshold, // Threshold coordinate value at which to clip the triangle
        bool keepAbove, // If true, keep portion of the triangle above threhold; else keep it below
        double u0, // Coordinate of first vertex in triangle (CCW order)
        double u1, // Coordinate of second vertex in triangle (CCW order)
        double u2, // Coordinate of third vertex in triangle (CCW order)
        std::vector<TriangleClipVertex>& result // The aray into which to copy the result.
    ) {
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
                result.emplace_back(TriangleClipVertex(1));
                result.emplace_back(TriangleClipVertex(2));
                if (u02Ratio != 1.0) {
                    result.emplace_back(TriangleClipVertex(0, 2, u02Ratio));
                }
                if (u01Ratio != 1.0) {
                    result.emplace_back(TriangleClipVertex(0, 1, u01Ratio));
                }
            } else if (u1Behind) {
                u12Ratio = (threshold - u1) / (u2 - u1);
                u10Ratio = (threshold - u1) / (u0 - u1);
                result.emplace_back(TriangleClipVertex(2));
                result.emplace_back(TriangleClipVertex(0));
                if (u10Ratio != 1.0) {
                    result.emplace_back(TriangleClipVertex(1, 0, u10Ratio));
                }
                if (u12Ratio != 1.0) {
                    result.emplace_back(TriangleClipVertex(1, 2, u12Ratio));
                }
            } else if (u2Behind) {
                u20Ratio = (threshold - u2) / (u0 - u2);
                u21Ratio = (threshold - u2) / (u1 - u2);
                result.emplace_back(TriangleClipVertex(0));
                result.emplace_back(TriangleClipVertex(1));
                if (u21Ratio != 1.0) {
                    result.emplace_back(TriangleClipVertex(2, 1, u21Ratio));
                }
                if (u20Ratio != 1.0) {
                    result.emplace_back(TriangleClipVertex(2, 0, u20Ratio));
                }
            }
        } else if (numberOfBehind == 2) {
            if (!u0Behind && u0 != threshold) {
                u10Ratio = (threshold - u1) / (u0 - u1);
                u20Ratio = (threshold - u2) / (u0 - u2);
                result.emplace_back(TriangleClipVertex(0));
                result.emplace_back(TriangleClipVertex(1, 0, u10Ratio));
                result.emplace_back(TriangleClipVertex(2, 0, u20Ratio));
            } else if (!u1Behind && u1 != threshold) {
                u21Ratio = (threshold - u2) / (u1 - u2);
                u01Ratio = (threshold - u0) / (u1 - u0);
                result.emplace_back(TriangleClipVertex(1));
                result.emplace_back(TriangleClipVertex(2, 1, u21Ratio));
                result.emplace_back(TriangleClipVertex(0, 1, u01Ratio));
            } else if (!u2Behind && u2 != threshold) {
                u02Ratio = (threshold - u0) / (u2 - u0);
                u12Ratio = (threshold - u1) / (u2 - u1);
                result.emplace_back(TriangleClipVertex(2));
                result.emplace_back(TriangleClipVertex(0, 2, u02Ratio));
                result.emplace_back(TriangleClipVertex(1, 2, u12Ratio));
            }
        } else if (numberOfBehind != 3) {
            // Completely in front of threshold
            result.emplace_back(TriangleClipVertex(0));
            result.emplace_back(TriangleClipVertex(1));
            result.emplace_back(TriangleClipVertex(2));
        }
        // else completely behind threshold

        return;
    }

}
