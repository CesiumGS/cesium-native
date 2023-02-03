#pragma once

#include "Library.h"

#include <glm/vec3.hpp>
//#include <vector>

namespace CesiumGeospatial {

	class CESIUMGEOSPATIAL_API SunPosition {
	public:

		static glm::dvec3 getSunAngle(float time);

	};
}
