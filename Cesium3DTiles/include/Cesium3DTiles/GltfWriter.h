#pragma once

#include "Gltf.h"
#include "GltfAccessor.h"

#include <stdexcept>

namespace Cesium3DTiles {

	template <class T>
	class GltfWriter {
	private:
		GltfAccessor<T> _accessor;

	public:
		GltfWriter(tinygltf::Model& model, size_t accessorID) :
			_accessor(model, accessorID)
		{
		}

		const T& operator[](size_t i) const {
			return this->_accessor[i];
		}

		T& operator[](size_t i) {
			return const_cast<T&>(this->_accessor[i]);
		}

		size_t size() const {
			return this->_accessor.size();
		}

		const tinygltf::Buffer& gltfBuffer() const
		{
			return this->_accessor.gltfBuffer();
		}

		tinygltf::Buffer& gltfBuffer()
		{
			return const_cast<tinygltf::Buffer&>(this->_accessor.gltfBuffer());
		}

		const tinygltf::BufferView& gltfBufferView() const
		{
			return this->_accessor.gltfBufferView();
		}

		tinygltf::BufferView& gltfBufferView() {
			return const_cast<tinygltf::BufferView&>(this->_accessor.gltfBufferView());
		}

		const tinygltf::Accessor& gltfAccessor() const {
			return this->_accessor.gltfAccessor();
		}

		tinygltf::Accessor& gltfAccessor() {
			return const_cast<tinygltf::Accessor&>(this->_accessor.gltfAccessor());
		}
	};

}