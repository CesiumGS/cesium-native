#include "CesiumGltf/Helpers.h"
#include "CesiumGltf/BufferView.h"

namespace CesiumGltf {

int8_t computeNumberOfComponents(CesiumGltf::Accessor::Type type) {
	switch (type) {
		case CesiumGltf::Accessor::Type::SCALAR:
			return 1;
		case CesiumGltf::Accessor::Type::VEC2:
			return 2;
		case CesiumGltf::Accessor::Type::VEC3:
			return 3;
		case CesiumGltf::Accessor::Type::VEC4:
			return 4;
		case CesiumGltf::Accessor::Type::MAT2:
			return 4;
		case CesiumGltf::Accessor::Type::MAT3:
			return 9;
		case CesiumGltf::Accessor::Type::MAT4:
			return 16;
		default:
			return 0;
	}
}

int8_t computeByteSizeOfComponent(CesiumGltf::Accessor::ComponentType componentType) {
	switch (componentType) {
		case CesiumGltf::Accessor::ComponentType::BYTE:
		case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
			return 1;
		case CesiumGltf::Accessor::ComponentType::SHORT:
		case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
			return 2;
		case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
		case CesiumGltf::Accessor::ComponentType::FLOAT:
			return 4;
		default:
			return 0;
	}
}

int64_t computeByteStride(const CesiumGltf::Accessor& accessor, const CesiumGltf::BufferView& bufferView) {
	if (bufferView.byteStride) {
		return bufferView.byteStride.value();
	} else {
		return computeNumberOfComponents(accessor.type) * computeByteSizeOfComponent(accessor.componentType);
	}
}

}