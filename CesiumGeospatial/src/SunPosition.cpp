#include "CesiumGeospatial/SunPosition.h"

namespace CesiumGeospatial {

	/*static*/ glm::dvec3 SunPosition::getSunAngle(float time) {
		return glm::dvec3(((time * 15.0) - 90.0f), 0.0, 0.0);
	 }
}
