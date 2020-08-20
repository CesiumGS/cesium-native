#include "Cesium3DTiles/GltfContent.h"
#include <stdexcept>
#include "tiny_gltf.h"

namespace Cesium3DTiles {

	std::string GltfContent::TYPE = "gltf";

	GltfContent::GltfContent(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& /*url*/) :
		TileContent(tile)
	{
		tinygltf::TinyGLTF loader;
		std::string errors;
		std::string warnings;

		/*bool loadSucceeded =*/ loader.LoadBinaryFromMemory(&this->_gltf, &errors, &warnings, data.data(), static_cast<unsigned int>(data.size()));
		//if (!loadSucceeded) {
		//	throw std::runtime_error("Failed to load glTF model from B3DM.");
		//}
	}

	GltfContent::GltfContent(const Tile& tile, tinygltf::Model&& data, const std::string& /*url*/) :
		TileContent(tile),
		_gltf(std::move(data))
	{
	}

}
