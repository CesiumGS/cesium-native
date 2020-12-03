#pragma once

#include "Gltf.h"
#include <stdexcept>

namespace Cesium3DTiles {

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
	 * tinygltf::Model model;
	 * GltfAccessor<glm::vec3> positions(model, accessorIndex)
	 * glm::vec3 position = positions[i];
	 * ```
	 *
	 * @tparam T The type of the elements in the accessor.
	 */
	template <class T>
	class GltfAccessor {
	private:
		const tinygltf::Buffer* _pGltfBuffer;
		const tinygltf::BufferView* _pGltfBufferView;
		const tinygltf::Accessor* _pGltfAccessor;
		const unsigned char* _pBufferViewData;
		size_t _stride;
		size_t _offset;
		size_t _size;

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
		GltfAccessor(const tinygltf::Model& model, size_t accessorID)
		{
			const tinygltf::Accessor& accessor = model.accessors[accessorID];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const std::vector<unsigned char>& data = buffer.data;
			size_t bufferBytes = data.size();
			if (bufferView.byteOffset + bufferView.byteLength > bufferBytes)
			{
				throw std::runtime_error("bufferView does not fit in buffer.");
			}

			int accessorByteStride = accessor.ByteStride(bufferView);
			if (accessorByteStride == -1)
			{
				throw std::runtime_error("cannot compute accessor byteStride.");
			}

			int32_t accessorComponentElements = tinygltf::GetNumComponentsInType(accessor.type);
			int32_t accessorComponentBytes = tinygltf::GetComponentSizeInBytes(accessor.componentType);
			int32_t accessorBytesPerStride = accessorComponentElements * accessorComponentBytes;

			if (sizeof(T) != accessorBytesPerStride)
			{
				throw std::runtime_error("sizeof(T) does not much accessor bytes.");
			}

			size_t accessorBytes = accessorByteStride * accessor.count;
			size_t bytesRemainingInBufferView = bufferView.byteLength - (accessor.byteOffset + accessorByteStride * (accessor.count - 1) + accessorBytesPerStride);
			if (accessorBytes > bufferView.byteLength || bytesRemainingInBufferView < 0)
			{
				throw std::runtime_error("accessor does not fit in bufferView.");
			}

			const unsigned char* pBufferData = &data[0];
			const unsigned char* pBufferViewData = pBufferData + bufferView.byteOffset;

			this->_pGltfBuffer = &buffer;
			this->_pGltfBufferView = &bufferView;
			this->_pGltfAccessor = &accessor;
			this->_stride = accessorByteStride;
			this->_offset = accessor.byteOffset;
			this->_size = accessor.count;
			this->_pBufferViewData = pBufferViewData;
		}

		/**
		 * @brief Provides the specified accessor element.
		 * 
		 * @param i The index of the element.
		 * @returns The constant reference to the accessor element.
		 * @throws A `std::range_error` if the given index is negative
		 * or not smaller than the {@link size} of this accessor.
		 */
		const T& operator[](size_t i) const
		{
			if (i < 0 || i >= this->_size)
			{
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
		size_t size() const noexcept
		{
			return this->_size;
		}

		/**
		 * @brief Returns the underyling buffer implementation.
		 */
		const tinygltf::Buffer& gltfBuffer() const
		{
			return *this->_pGltfBuffer;
		}

		/**
		 * @brief Returns the underyling buffer view implementation.
		 */
		const tinygltf::BufferView& gltfBufferView() const noexcept
		{
			return *this->_pGltfBufferView;
		}

		/**
		 * @brief Returns the underyling acessor implementation.
		 */
		const tinygltf::Accessor& gltfAccessor() const noexcept
		{
			return *this->_pGltfAccessor;
		}
	};

}