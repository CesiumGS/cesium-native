#include "CesiumGeospatial/SunPosition.h"

namespace CesiumGeospatial {

	/*static*/ glm::dvec3 SunPosition::getSunAngle(float time) {

		// North offset is a temporary approximation to align the sun position east to west.
		float northOffset = -90.0;

		return glm::dvec3(((time * 15.0) - 90.0f), northOffset, 0.0);
	 }
}
