#include "CesiumGltf/GltfModel.h"
#include "CesiumGltf/GltfMesh.h"
#include <stdexcept>

namespace CesiumGltf {

    /*static*/ GltfModel GltfModel::fromMemory(gsl::span<const uint8_t> data) {
        GltfModel result;
        std::string warnings;
        std::string errors;
        tinygltf::TinyGLTF loader;
        /*bool success =*/ loader.LoadBinaryFromMemory(&result._model, &errors, &warnings, data.data(), static_cast<unsigned int>(data.size()));
        return result;
        // cgltf_options options{};
        // cgltf_data* pResult = nullptr;
        // cgltf_result resultCode = cgltf_parse(&options, data.data(), data.size(), &pResult);
        // if (resultCode == cgltf_result_success) {
        //     return GltfModel(pResult);
        // } else {
        //     switch (resultCode) {
        //         case cgltf_result_data_too_short:
        //             throw std::runtime_error("glTF data is too short.");
        //         case cgltf_result_unknown_format:
        //             throw std::runtime_error("glTF is in an unknown format.");
        //         case cgltf_result_invalid_json:
        //             throw std::runtime_error("glTF JSON is invalid.");
        //         case cgltf_result_invalid_gltf:
        //             throw std::runtime_error("glTF is invalid.");
        //         case cgltf_result_invalid_options:
        //             throw std::runtime_error("glTF parsed with invalid options.");
        //         case cgltf_result_file_not_found:
        //             throw std::runtime_error("glTF file was not found.");
        //         case cgltf_result_io_error:
        //             throw std::runtime_error("I/O error while parsing glTF.");
        //         case cgltf_result_out_of_memory:
        //             throw std::runtime_error("Out of memory while parsing glTF.");
        //         case cgltf_result_legacy_gltf:
        //             throw std::runtime_error("glTF uses an unsupported legacy format.");
        //         default:
        //             throw std::runtime_error("An unknown error occurred while parsing a glTF.");
        //     }
        // }
    }

	// cgltf_asset asset;

	// cgltf_mesh* meshes;
	// cgltf_material* materials;
	// cgltf_accessor* accessors;
	// cgltf_buffer_view* buffer_views;
	// cgltf_buffer* buffers;
	// cgltf_image* images;
	// cgltf_texture* textures;
	// cgltf_sampler* samplers;
	// cgltf_skin* skins;
	// cgltf_camera* cameras;
	// cgltf_light* lights;
	// cgltf_node* nodes;
	// cgltf_scene* scenes;
	// cgltf_scene* scene;
	// cgltf_animation* animations;

    // GltfCollection<const std::string> GltfModel::extensionsUsed() const noexcept {
    //     return GltfCollection<const std::string>(&this->_model.extensionsUsed);
    // }

    // GltfCollection<std::string> GltfModel::extensionsRequired() const noexcept {
    //     return { &this->_model.extensionsRequired };
    // }

    // GltfCollection<GltfMesh> GltfModel::meshes() const noexcept {
    //     return { &this->_pData->meshes, &this->_pData->meshes_count };
    // }

    // GltfCollection<GltfAccessor> GltfModel::accessors() const noexcept {
    //     return { &this->_pData->accessors, &this->_pData->accessors_count };
    // }

    // GltfModel::GltfModel(tinygltf::Model&& model) :
    //     _model(std::move(model))
    // {
    // }
}
