#pragma once

#include "Gltf.h"
#include "GltfAccessor.h"

#include <stdexcept>

namespace Cesium3DTiles {

	/**
	 * @brief Provides write access to a {@link GltfAccessor}.
	 */
	template <class T>
	class GltfWriter final {
	private:
		GltfAccessor<T> _accessor;

	public:

		/** @copydoc GltfAccessor::GltfAccessor */
		GltfWriter(CesiumGltf::Model& model, size_t accessorID) :
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
		const CesiumGltf::Buffer& gltfBuffer() const noexcept
		{
			return this->_accessor.gltfBuffer();
		}

		/** @copydoc GltfAccessor::gltfBuffer */
		CesiumGltf::Buffer& gltfBuffer() noexcept
		{
			return const_cast<CesiumGltf::Buffer&>(this->_accessor.gltfBuffer());
		}

		/** @copydoc GltfAccessor::gltfBufferView */
		const CesiumGltf::BufferView& gltfBufferView() const noexcept
		{
			return this->_accessor.gltfBufferView();
		}

		/** @copydoc GltfAccessor::gltfBufferView */
		CesiumGltf::BufferView& gltfBufferView() noexcept {
			return const_cast<CesiumGltf::BufferView&>(this->_accessor.gltfBufferView());
		}

		/** @copydoc GltfAccessor::gltfAccessor */
		const CesiumGltf::Accessor& gltfAccessor() const noexcept {
			return this->_accessor.gltfAccessor();
		}

		/** @copydoc GltfAccessor::gltfAccessor */
		CesiumGltf::Accessor& gltfAccessor() noexcept {
			return const_cast<CesiumGltf::Accessor&>(this->_accessor.gltfAccessor());
		}
	};

}