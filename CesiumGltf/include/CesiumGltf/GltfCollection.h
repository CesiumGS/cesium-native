#pragma once

#include "CesiumGltf/CgltfMapping.h"
#include <optional>
#include <string>

namespace CesiumGltf {
    template <typename T>
    class GltfCollection {
    public:
        class const_iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = int64_t;
            using pointer = T*;
            using reference = const value_type&;

            const_iterator(typename Impl::CesiumToCgltf<T>::cgltf_type* pElements, size_t numberOfElements, size_t currentElement) noexcept :
                _pElements(pElements),
                _numberOfElements(numberOfElements),
                _currentElement(currentElement),
                _temporary()
            {
            }

            const T& operator*() const noexcept {
                this->_fill();
                return this->_temporary.value();
            }

            const T* operator->() const {
                this->_fill();
                return &this->_temporary.value();
            }

            const_iterator& operator++() noexcept {
                ++this->_currentElement;
                this->_temporary.reset();
                return *this;
            }

            const_iterator operator++(int) noexcept {
                const_iterator copy = *this;
                ++*this;
                this->_temporary.reset();
                return copy;
            }

            bool operator==(const const_iterator& rhs) const noexcept {
                return
                    this->_currentElement == rhs._currentElement &&
                    this->_pElements == rhs._pElements;
            }

            bool operator!=(const const_iterator& rhs) const noexcept {
                return !(*this == rhs);
            }

        private:
            void _fill() const {
                if (!this->_temporary) {
                    assert(this->_currentElement < this->_numberOfElements);
                    this->_temporary = Impl::CesiumGltfObjectFactory<T>::createFromCollectionElement(this->_pElements, this->_currentElement);
                }
            }

            typename Impl::CesiumToCgltf<T>::cgltf_type* _pElements;
            size_t _numberOfElements;
            size_t _currentElement;
            mutable std::optional<T> _temporary;
        };

        GltfCollection(typename Impl::CesiumToCgltf<T>::cgltf_type** ppElements, size_t* pNumberOfElements) :
            _ppElements(ppElements),
            _pNumberOfElements(pNumberOfElements)
        {
        }

        using value_type = T;

        size_t size() const noexcept {
            return *this->_pNumberOfElements;
        }

        const_iterator begin() noexcept {
            return {
                *this->_ppElements,
                *this->_pNumberOfElements,
                0
            };
        }

        const_iterator end() noexcept {
            return {
                *this->_ppElements,
                *this->_pNumberOfElements,
                *this->_pNumberOfElements
            };
        }

        T operator[](size_t index) const noexcept {
            return Impl::CesiumGltfObjectFactory<T>::createFromCollectionElement(*this->_ppElements, index);
        }

        void push_back(const T& item) {
            size_t newNumberOfElements = *this->_pNumberOfElements + 1;
            *this->_ppElements = realloc(*this->_ppElements, newNumberOfElements * sizeof(Impl::CesiumToCgltf<T>::cgltf_type));
            (*this->_ppElements)[*this->_pNumberOfElements] = item;
            *this->_pNumberOfElements = newNumberOfElements;
        }

        void push_back(T&& item) {
            size_t newNumberOfElements = *this->_pNumberOfElements + 1;
            *this->_ppElements = realloc(*this->_ppElements, newNumberOfElements * sizeof(Impl::CesiumToCgltf<T>::cgltf_type));
            (*this->_ppElements)[*this->_pNumberOfElements] = std::move(item);
            *this->_pNumberOfElements = newNumberOfElements;
        }

        T emplace_back() {
            size_t oldNumberOfElements = *this->_pNumberOfElements;
            size_t newNumberOfElements = oldNumberOfElements + 1;
            *this->_ppElements = realloc(*this->_ppElements, newNumberOfElements * sizeof(Impl::CesiumToCgltf<T>::cgltf_type));
            (*this->_ppElements)[oldNumberOfElements] = Impl::CesiumToCgltf<T>::cgltf_type();
            *this->_pNumberOfElements = newNumberOfElements;
            return this->operator[](oldNumberOfElements);
        }        

    private:
        typename Impl::CesiumToCgltf<T>::cgltf_type** _ppElements;
        size_t* _pNumberOfElements;
    };

}
