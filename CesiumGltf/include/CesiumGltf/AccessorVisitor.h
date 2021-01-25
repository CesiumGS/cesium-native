#pragma once

#include "CesiumGltf/AccessorView.h"

namespace CesiumGltf {

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
            T value[9];
        };

        #pragma pack(pop)
    };

    template <typename TVisitor>
    class AccessorVisitor {
    public:
        using ResultType = std::invoke_result_t<TVisitor, int64_t, float>;

		/**
		 * @brief Construct a new instance not pointing to any data.
		 * 
		 * The new instance will have a {@link size} of 0 and a {@link status} of `AccessorViewStatus::InvalidAccessorIndex`.
		 */
        AccessorVisitor(TVisitor&& visitor) :
            _visitor(std::forward<TVisitor>(visitor)),
            _pDispatcher(std::make_unique<Dispatcher<float>>(AccessorView<float>())),
            _size(0)
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
         * @param visitor The functor to receive callbacks on calls to {@link visit}.
		 */
		AccessorVisitor(const Model& model, const Accessor& accessor, TVisitor&& visitor) noexcept :
            AccessorVisitor(std::forward<TVisitor>(visitor))
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
         * @param visitor The functor to receive callbacks on calls to {@link visit}.
		 */
		AccessorVisitor(const Model& model, int32_t accessorIndex, TVisitor&& visitor) noexcept :
            AccessorVisitor(std::forward<TVisitor>(visitor))
        {
			const Accessor* pAccessor = Model::getSafe(&model.accessors, accessorIndex);
			if (!pAccessor) {
				return;
			}

			this->create(model, *pAccessor);

            this->_size = this->_pDispatcher->size();
		}

        ResultType visit(int64_t i) {
            return this->_pDispatcher->dispatch(this->_visitor, i);
        }

        int64_t size() const noexcept {
            return this->_size;
        }

        TVisitor& getVisitor() noexcept {
            return this->_visitor;
        }

        const TVisitor& getVisitor() const noexcept {
            return this->_visitor;
        }

    private:
        class IDispatcher {
        public:
            virtual ~IDispatcher() = default;
            virtual ResultType dispatch(TVisitor& visitor, int64_t i) = 0;
            virtual int64_t size() const = 0;
        };

        template <typename T>
        class Dispatcher : public IDispatcher {
        public:
            Dispatcher(const AccessorView<T>& view) :
                _view(view)
            {
            }

            virtual ResultType dispatch(TVisitor& visitor, int64_t i) override {
                return visitor(i, this->_view[i]);
            }

            virtual int64_t size() const override {
                return this->_view.size();
            }

        private:
            AccessorView<T> _view;
        };

        template <typename T>
        void create(const Model& model, const Accessor& accessor) {
            switch (accessor.type) {
            case Accessor::Type::SCALAR:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::SCALAR<T>>>(AccessorView<AccessorTypes::SCALAR<T>>(model, accessor));
                break;
            case Accessor::Type::VEC2:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::VEC2<T>>>(AccessorView<AccessorTypes::VEC2<T>>(model, accessor));
                break;
            case Accessor::Type::VEC3:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::VEC3<T>>>(AccessorView<AccessorTypes::VEC3<T>>(model, accessor));
                break;
            case Accessor::Type::VEC4:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::VEC4<T>>>(AccessorView<AccessorTypes::VEC4<T>>(model, accessor));
                break;
            case Accessor::Type::MAT2:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::MAT2<T>>>(AccessorView<AccessorTypes::MAT2<T>>(model, accessor));
                break;
            case Accessor::Type::MAT3:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::MAT3<T>>>(AccessorView<AccessorTypes::MAT3<T>>(model, accessor));
                break;
            case Accessor::Type::MAT4:
                this->_pDispatcher = std::make_unique<Dispatcher<AccessorTypes::MAT4<T>>>(AccessorView<AccessorTypes::MAT4<T>>(model, accessor));
                break;
            }
        }

        void create(const Model& model, const Accessor& accessor) {
            switch (accessor.componentType) {
            case Accessor::ComponentType::BYTE:
                this->create<int8_t>(model, accessor);
                break;
            case Accessor::ComponentType::UNSIGNED_BYTE:
                this->create<uint8_t>(model, accessor);
                break;
            case Accessor::ComponentType::SHORT:
                this->create<int16_t>(model, accessor);
                break;
            case Accessor::ComponentType::UNSIGNED_SHORT:
                this->create<uint16_t>(model, accessor);
                break;
            case Accessor::ComponentType::UNSIGNED_INT:
                this->create<uint32_t>(model, accessor);
                break;
            case Accessor::ComponentType::FLOAT:
                this->create<float>(model, accessor);
                break;
            }
        }

        // TODO: All Dispatcher<T> will be the same size, so we should reserve
        // that much space and use placement new to avoid a heap allocation.
        TVisitor _visitor;
        std::unique_ptr<IDispatcher> _pDispatcher;
        int64_t _size;
    };

    template <typename TVisitor>
    AccessorVisitor<TVisitor> createAccessorVisitor(TVisitor&& visitor) {
        return AccessorVIsitor<TVisitor>(std::forward<TVisitor>(visitor));
    }

    template <typename TVisitor>
    AccessorVisitor<TVisitor> createAccessorVisitor(const Model& model, const Accessor& accessor, TVisitor&& visitor) {
        return AccessorVIsitor<TVisitor>(model, accessor, std::forward<TVisitor>(visitor));
    }

    template <typename TVisitor>
    AccessorVisitor<TVisitor> createAccessorVisitor(const Model& model, int32_t accessorIndex, TVisitor&& visitor) {
        return AccessorVisitor<TVisitor>(model, accessorIndex, std::forward<TVisitor>(visitor));
    }

}
