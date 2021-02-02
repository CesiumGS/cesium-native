#pragma once

#include "CesiumGltf/Model.h"
#include <stdexcept>

namespace CesiumGltf {
	/**
	 * @brief Indicates the status of an {@link AccessorView}.
	 * 
	 * The {@link AccessorView} constructor always completes successfully. However, it may not
	 * always reflect the actual content of the {@link Accessor}, but instead indicate that
	 * its {@link AccessorView::size} is 0. This enumeration provides the reason.
	 */
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
	 * parameter. Instances are usually from an {@link Accessor},
	 * and the {@link operator[]()} can be used to access the elements:
	 * 
	 * @snippet TestAccessorView.cpp createFromAccessorAndRead
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

		/**
		 * @brief Construct a new instance not pointing to any data.
		 * 
		 * The new instance will have a {@link size} of 0 and a {@link status} of `AccessorViewStatus::InvalidAccessorIndex`.
		 */
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

		/**
		 * @brief Creates a new instance from a given model and {@link Accessor}.
		 * 
		 * If the accessor cannot be viewed, the construct will still complete successfully
		 * without throwing an exception. However, {@link size} will return 0 and
		 * {@link status} will indicate what went wrong.
		 * 
		 * @param model The model to access.
		 * @param accessor The accessor to view.
		 */
		AccessorView(const Model& model, const Accessor& accessor) noexcept :
			AccessorView()
		{
			this->create(model, accessor);
		}

		/**
		 * @brief Creates a new instance from a given model and accessor index.
		 * 
		 * If the accessor cannot be viewed, the construct will still complete successfully
		 * without throwing an exception. However, {@link size} will return 0 and
		 * {@link status} will indicate what went wrong.
		 * 
		 * @param model The model to access.
		 * @param accessorIndex The index of the accessor to view in the model's {@link Model::accessors} list.
		 */
		AccessorView(const Model& model, int32_t accessorIndex) noexcept :
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

	/**
	 * @brief Contains types that may optionally be used with {@link AccessorView} for various {@link Accessor::componentType} values.
	 */
    struct AccessorTypes {
        #pragma pack(push, 1)

        template <typename T>
        struct SCALAR {
            T value[1];
        };

        template <typename T>
        struct VEC2 {
            T value[2];
        };

        template <typename T>
        struct VEC3 {
            T value[3];
        };

        template <typename T>
        struct VEC4 {
            T value[4];
        };

        template <typename T>
        struct MAT2 {
            T value[4];
        };

        template <typename T>
        struct MAT3 {
            T value[9];
        };

        template <typename T>
        struct MAT4 {
            T value[16];
        };

        #pragma pack(pop)
    };

	namespace Impl {
		template <typename TCallback, typename TElement>
		std::invoke_result_t<TCallback, AccessorView<float>> createAccessorView(const Model& model, const Accessor& accessor, TCallback&& callback) {
            switch (accessor.type) {
            case Accessor::Type::SCALAR:
				return callback(AccessorView<AccessorTypes::SCALAR<TElement>>(model, accessor));
            case Accessor::Type::VEC2:
				return callback(AccessorView<AccessorTypes::VEC2<TElement>>(model, accessor));
            case Accessor::Type::VEC3:
				return callback(AccessorView<AccessorTypes::VEC3<TElement>>(model, accessor));
            case Accessor::Type::VEC4:
				return callback(AccessorView<AccessorTypes::VEC4<TElement>>(model, accessor));
            case Accessor::Type::MAT2:
				return callback(AccessorView<AccessorTypes::MAT2<TElement>>(model, accessor));
            case Accessor::Type::MAT3:
				return callback(AccessorView<AccessorTypes::MAT3<TElement>>(model, accessor));
            case Accessor::Type::MAT4:
				return callback(AccessorView<AccessorTypes::MAT4<TElement>>(model, accessor));
			default:
				return callback(AccessorView<std::nullptr_t>());
            }
		}
	}

	/**
	 * @brief Creates an appropriate {@link AccessorView} for a given accessor.
	 * 
	 * The created accessor is provided via a callback, which is a function that can be
	 * invoked with all possible {@link AccessorView} types. If an accessor cannot be
	 * created, the callback will be invoked with `AccessorView<std::nullptr_t>`.
	 * 
	 * @tparam TCallback The callback.
	 * @param model The model to access.
 	 * @param accessor The accessor to view.
	 * @param callback The callback that receives the created accessor.
	 * @return The value returned by the callback.
	 */
	template <typename TCallback>
	std::invoke_result_t<TCallback, AccessorView<float>> createAccessorView(const Model& model, const Accessor& accessor, TCallback&& callback) {
		switch (accessor.componentType) {
		case Accessor::ComponentType::BYTE:
			return ::CesiumGltf::Impl::createAccessorView<TCallback, int8_t>(model, accessor, std::forward<TCallback>(callback));
		case Accessor::ComponentType::UNSIGNED_BYTE:
			return ::CesiumGltf::Impl::createAccessorView<TCallback, uint8_t>(model, accessor, std::forward<TCallback>(callback));
		case Accessor::ComponentType::SHORT:
			return ::CesiumGltf::Impl::createAccessorView<TCallback, int16_t>(model, accessor, std::forward<TCallback>(callback));
		case Accessor::ComponentType::UNSIGNED_SHORT:
			return ::CesiumGltf::Impl::createAccessorView<TCallback, uint16_t>(model, accessor, std::forward<TCallback>(callback));
		case Accessor::ComponentType::UNSIGNED_INT:
			return ::CesiumGltf::Impl::createAccessorView<TCallback, uint32_t>(model, accessor, std::forward<TCallback>(callback));
		case Accessor::ComponentType::FLOAT:
			return ::CesiumGltf::Impl::createAccessorView<TCallback, float>(model, accessor, std::forward<TCallback>(callback));
		default:
			return callback(AccessorView<std::nullptr_t>());
		}
	}

	/**
	 * @brief Creates an appropriate {@link AccessorView} for a given accessor.
	 * 
	 * The created accessor is provided via a callback, which is a function that can be
	 * invoked with all possible {@link AccessorView} types. If an accessor cannot be
	 * created, the callback will be invoked with `AccessorView<std::nullptr_t>`.
	 * 
	 * @tparam TCallback The callback.
	 * @param model The model to access.
 	 * @param accessorIndex The index of the accessor to view in {@link Model::accessors}.
	 * @param callback The callback that receives the created accessor.
	 * @return The value returned by the callback.
	 */
	template <typename TCallback>
	std::invoke_result_t<TCallback, AccessorView<float>> createAccessorView(const Model& model, int32_t accessorIndex, TCallback&& callback) {
		const Accessor* pAccessor = Model::getSafe(&model.accessors, accessorIndex);
		if (!pAccessor) {
			return callback(AccessorView<std::nullptr_t>());
		}

		return createAccessorView(model, *pAccessor, callback);
	}
}
