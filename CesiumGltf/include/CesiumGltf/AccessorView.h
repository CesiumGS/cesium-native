#pragma once

#include "CesiumGltf/Model.h"
#include <stdexcept>

namespace CesiumGltf {

	enum class AccessorViewStatus {
		/**
		 * @brief This accessor is valid and ready to use.
		 */
		Valid,

		/**
		 * @brief The accessor index does not refer to a valid accessor.
		 */
		InvalidAccessorIndex,

		/**
		 * @brief The accessor's bufferView index does not refer to a valid bufferView.
		 */
		InvalidBufferViewIndex,

		/**
		 * @brief The accessor's bufferView's buffer index does not refer to a valid buffer.
		 */
		InvalidBufferIndex,

		/**
		 * @brief The accessor is too large to fit in its bufferView.
		 */
		BufferViewTooSmall,

		/**
		 * @brief The accessor's bufferView is too large to fit in its buffer.
		 */
		BufferTooSmall,

		/**
		 * @brief The `sizeof(T)` does not match the accessor's {@link Accessor::computeBytesPerVertex}.
		 */
		WrongSizeT,
	};

	/**
	 * @brief A view on the data of one accessor of a glTF asset.
	 * 
	 * It provides the actual accessor data like an array of elements.
	 * The type of the accessor elements is determined by the template 
	 * parameter. Instances are usually created by {@link Accessor::createView},
	 * and the {@link operator[]()} can be used to access the elements:
	 * 
	 * ```
	 * CesiumGltf::Model model;
	 * AccessorView<glm::vec3> positions = model.accessors[0].createView<glm::vec3)(model);
	 * glm::vec3 position = positions[i];
	 * ```
	 *
	 * @tparam T The type of the elements in the accessor.
	 */
	template <class T>
	class AccessorView final {
	private:
        const uint8_t* _pData;
		int64_t _stride;
		int64_t _offset;
		int64_t _size;
		AccessorViewStatus _status;

	public:
		typedef T value_type;

		AccessorView() :
			_pData(nullptr),
			_stride(0),
			_offset(0),
			_size(0),
			_status(AccessorViewStatus::InvalidAccessorIndex)
		{
		}

		/**
		 * @brief Creates a new instance from low-level parameters.
		 * 
		 * The provided parameters are not validated in any way, and so this overload can easily
		 * be used to access invalid memory.
		 * 
		 * @param pData The raw data buffer from which to read.
		 * @param stride The stride, in bytes, between successive elements.
		 * @param offset The offset from the start of the buffer to the first element.
		 * @param size The total number of elements.
		 */
		AccessorView(const uint8_t* pData, int64_t stride, int64_t offset, int64_t size) :
			_pData(pData),
			_stride(stride),
			_offset(offset),
			_size(size),
			_status(AccessorViewStatus::Valid)
		{
		}

		AccessorView(const Model& model, const Accessor& accessor) :
			AccessorView()
		{
			this->create(model, accessor);
		}

		AccessorView(const Model& model, int32_t accessorIndex) :
			AccessorView()
		{
			const Accessor* pAccessor = Model::getSafe(&model.accessors, accessorIndex);
			if (!pAccessor) {
				this->_status = AccessorViewStatus::InvalidAccessorIndex;
				return;
			}

			this->create(model, *pAccessor);
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

			return *reinterpret_cast<const T*>(this->_pData + i * this->_stride + this->_offset);
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

		/**
		 * @brief Gets the status of this accessor view.
		 * 
		 * Indicates whether the view accurately reflects the accessor's data, or whether
		 * an error occurred.
		 */
		AccessorViewStatus status() const noexcept {
			return this->_status;
		}

	private:
		void create(const Model& model, const Accessor& accessor) {
			const CesiumGltf::BufferView* pBufferView = Model::getSafe(&model.bufferViews, accessor.bufferView);
			if (!pBufferView) {
				this->_status = AccessorViewStatus::InvalidBufferViewIndex;
				return;
			}

			const CesiumGltf::Buffer* pBuffer = Model::getSafe(&model.buffers, pBufferView->buffer);
			if (!pBuffer) {
				this->_status = AccessorViewStatus::InvalidBufferIndex;
				return;
			}

			const std::vector<uint8_t>& data = pBuffer->cesium.data;
			int64_t bufferBytes = data.size();
			if (pBufferView->byteOffset + pBufferView->byteLength > bufferBytes) {
                this->_status = AccessorViewStatus::BufferTooSmall;
                return;
			}

			int64_t accessorByteStride = accessor.computeByteStride(model);
			int64_t accessorComponentElements = accessor.computeNumberOfComponents();
			int64_t accessorComponentBytes = accessor.computeByteSizeOfComponent();
			int64_t accessorBytesPerStride = accessorComponentElements * accessorComponentBytes;

			if (sizeof(T) != accessorBytesPerStride) {
				this->_status = AccessorViewStatus::WrongSizeT;
                return;
			}

			int64_t accessorBytes = accessorByteStride * accessor.count;
			int64_t bytesRemainingInBufferView = pBufferView->byteLength - (accessor.byteOffset + accessorByteStride * (accessor.count - 1) + accessorBytesPerStride);
			if (accessorBytes > pBufferView->byteLength || bytesRemainingInBufferView < 0) {
				this->_status = AccessorViewStatus::BufferViewTooSmall;
                return;
			}

			this->_pData = pBuffer->cesium.data.data();
			this->_stride = accessorByteStride;
			this->_offset = accessor.byteOffset + pBufferView->byteOffset;
			this->_size = accessor.count;
			this->_status = AccessorViewStatus::Valid;
		}
	};

}
