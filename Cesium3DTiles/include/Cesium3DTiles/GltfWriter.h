#pragma once

#include "Gltf.h"
#include "GltfAccessor.h"

#include <stdexcept>

namespace Cesium3DTiles {

	/**
	 * @brief Provides write access to a {@link GltfAccessor}.
	 */
	template <class T>
	class GltfWriter {
	private:
		GltfAccessor<T> _accessor;

	public:

		/** @copydoc GltfAccessor::GltfAccessor */
		GltfWriter(tinygltf::Model& model, size_t accessorID) :
			_accessor(model, accessorID)
		{
		}

		/** @copydoc GltfAccessor::operator[]() */
		const T& operator[](size_t i) const {
			return this->_accessor[i];
		}

		/** @copydoc GltfAccessor::operator[]() */
		T& operator[](size_t i) {
			return const_cast<T&>(this->_accessor[i]);
		}

		/** @copydoc GltfAccessor::size */
		size_t size() const noexcept {
			return this->_accessor.size();
		}

		/** @copydoc GltfAccessor::gltfBuffer */
		const tinygltf::Buffer& gltfBuffer() const noexcept
		{
			return this->_accessor.gltfBuffer();
		}

		/** @copydoc GltfAccessor::gltfBuffer */
		tinygltf::Buffer& gltfBuffer() noexcept
		{
			return const_cast<tinygltf::Buffer&>(this->_accessor.gltfBuffer());
		}

		/** @copydoc GltfAccessor::gltfBufferView */
		const tinygltf::BufferView& gltfBufferView() const noexcept
		{
			return this->_accessor.gltfBufferView();
		}

		/** @copydoc GltfAccessor::gltfBufferView */
		tinygltf::BufferView& gltfBufferView() noexcept {
			return const_cast<tinygltf::BufferView&>(this->_accessor.gltfBufferView());
		}

		/** @copydoc GltfAccessor::gltfAccessor */
		const tinygltf::Accessor& gltfAccessor() const noexcept {
			return this->_accessor.gltfAccessor();
		}

		/** @copydoc GltfAccessor::gltfAccessor */
		tinygltf::Accessor& gltfAccessor() noexcept {
			return const_cast<tinygltf::Accessor&>(this->_accessor.gltfAccessor());
		}
	};

}