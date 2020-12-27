#pragma once

#include "CesiumGltf/TinyGltfMapping.h"
#include <optional>
#include <string>
#include <vector>

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

            const_iterator(typename Impl::CesiumToTinyGltf<T>::tinygltf_type* pElements, size_t numberOfElements, size_t currentElement) noexcept :
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

            std::vector<typename Impl::CesiumToTinyGltf<T>::tinygltf_type>* _pElements;
            size_t _currentElement;
            mutable std::optional<T> _temporary;
        };

        using vector_type = std::conditional_t<
            std::is_const<T>::value,
            std::vector<typename Impl::CesiumToTinyGltf<std::remove_const_t<T>>::tinygltf_type>,
            std::vector<typename Impl::CesiumToTinyGltf<std::remove_const_t<T>>::tinygltf_type>
        >;
        
        GltfCollection(vector_type* pElements) :
            _pElements(ppElements)
        {
        }

        using value_type = T;

        size_t size() const noexcept {
            return this->_pElements->size();
        }

        const_iterator begin() noexcept {
            return {
                this->_pElements,
                0
            };
        }

        const_iterator end() noexcept {
            return {
                this->_pElements,
                this->_pElements->size()
            };
        }

        T operator[](size_t index) const noexcept {
            return Impl::CesiumGltfObjectFactory<T>::createFromCollectionElement(&this->_pElements[index]);
        }

        void push_back(const T& item) {
            this->_pElements->push_back(*item._p);
        }

        void push_back(T&& item) {
            this->_pElements->push_back(std::move(*item._p));
        }

        T emplace_back() {
            return T(&this->_pElements->emplace_back());
        }        

    private:
        using tinygltf_type = typename Impl::CesiumToTinyGltf<T>::tinygltf_type;
        std::vector<tinygltf_type>* _pElements;
    };

}
