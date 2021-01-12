#pragma once

#include "Gltf.h"
#include "GltfAccessor.h"

#include <stdexcept>

namespace CesiumGltf {

	/**
	 * @brief Provides write access to an {@link AccessorView}.
	 */
	template <class T>
	class AccessorWriter final {
	private:
		AccessorView<T> _accessor;

	public:

		/** @copydoc AccessorView::AccessorView */
		AccessorWriter(CesiumGltf::Model& model, int32_t accessorID) :
			_accessor(model, accessorID)
		{
		}

		/** @copydoc AccessorView::operator[]() */
		const T& operator[](size_t i) const {
			return this->_accessor[i];
		}

		/** @copydoc AccessorView::operator[]() */
		T& operator[](size_t i) {
			return const_cast<T&>(this->_accessor[i]);
		}

		/** @copydoc AccessorView::size */
		size_t size() const noexcept {
			return this->_accessor.size();
		}
	};

}