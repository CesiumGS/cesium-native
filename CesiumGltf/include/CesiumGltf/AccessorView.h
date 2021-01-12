#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumGltf/Helpers.h"
#include <stdexcept>

namespace CesiumGltf {

	/**
	 * @brief A view on the data of one accessor of a glTF asset.
	 * 
	 * It provides the actual accessor data like an array of elements.
	 * The type of the accessor elements is determined by the template 
	 * parameter. Instances are created from an input glTF model
	 * and an accessor index, and the {@link operator[]()} 
	 * can be used to access the elements:
	 * 
	 * ```
	 * CesiumGltf::Model model;
	 * GltfAccessor<glm::vec3> positions(model, accessorIndex)
	 * glm::vec3 position = positions[i];
	 * ```
	 *
	 * @tparam T The type of the elements in the accessor.
	 */
	template <class T>
	class AccessorView final {
	private:
        uint8_t* _pData;
		int64_t _stride;
		int64_t _offset;
		int64_t _size;

	public:
		typedef T value_type;

		/**
		 * @brief Creates a new instance.
		 * 
		 * The resulting instance will provide the data of the specified
		 * accessor from the given model.
		 * 
		 * @param model The glTF model.
		 * @param accessorID The ID (index) of the accessor.
		 * @throws An `std::runtime_error` may be thrown when there are 
		 * inconsistencies in the given model. This may refer to the model
		 * itself, or to cases where the size of the template parameter `T`
		 * does not match the size of the elements of the specified accessor.
		 */
		AccessorView(const CesiumGltf::Model& model, int32_t accessorID) {
			const CesiumGltf::Accessor& accessor = model.accessors[accessorID];
			const CesiumGltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const CesiumGltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const std::vector<uint8_t>& data = buffer.cesium.data;
			int64_t bufferBytes = data.size();
			if (bufferView.byteOffset + bufferView.byteLength > bufferBytes) {
				throw std::runtime_error("bufferView does not fit in buffer.");
			}

			int64_t accessorByteStride = computeByteStride(accessor, bufferView);
			if (accessorByteStride == -1) {
				throw std::runtime_error("cannot compute accessor byteStride.");
			}

			int64_t accessorComponentElements = computeNumberOfComponents(accessor.type);
			int64_t accessorComponentBytes = computeByteSizeOfComponent(accessor.componentType);
			int64_t accessorBytesPerStride = accessorComponentElements * accessorComponentBytes;

			if (sizeof(T) != accessorBytesPerStride) {
				throw std::runtime_error("sizeof(T) does not much accessor bytes.");
			}

			int64_t accessorBytes = accessorByteStride * accessor.count;
			int64_t bytesRemainingInBufferView = bufferView.byteLength - (accessor.byteOffset + accessorByteStride * (accessor.count - 1) + accessorBytesPerStride);
			if (accessorBytes > bufferView.byteLength || bytesRemainingInBufferView < 0) {
				throw std::runtime_error("accessor does not fit in bufferView.");
			}

			this->_stride = accessorByteStride;
			this->_offset = accessor.byteOffset + bufferView.byteOffset;
			this->_size = accessor.count;
            this->_pData = &buffer.cesium.data.data();
		}

		/**
		 * @brief Provides the specified accessor element.
		 * 
		 * @param i The index of the element.
		 * @returns The constant reference to the accessor element.
		 * @throws A `std::range_error` if the given index is negative
		 * or not smaller than the {@link size} of this accessor.
		 */
		const T& operator[](int64_t i) const {
			if (i < 0 || i >= this->_size) {
				throw std::range_error("index out of range");
			}

			return *reinterpret_cast<const T*>(this->_pBufferViewData + i * this->_stride + this->_offset);
		}

		/**
		 * @brief Returns the size (number of elements) of this accessor. 
		 * 
		 * This is the number of elements of type `T` that this accessor contains.
		 * 
		 * @returns The size.
		 */
		int64_t size() const noexcept {
			return this->_size;
		}
	};

}
