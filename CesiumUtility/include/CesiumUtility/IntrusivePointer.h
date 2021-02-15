#pragma once

namespace CesiumUtility {

    /**
     * @brief A smart pointer that calls `addReference` and `releaseReference` on the controlled object.
     * 
     * @tparam T The type of object controlled.
     */
    template <class T>
    class IntrusivePointer final {
    public:

        /**
         * @brief Default constructor.
         */
        IntrusivePointer(T* p = nullptr) noexcept :
            _p(p)
        {
            this->addReference();
        }

        /**
         * @brief Copy constructor.
         */
        IntrusivePointer(const IntrusivePointer<T>& rhs) noexcept :
            _p(rhs._p)
        {
            this->addReference();
        }

        /**
         * @brief Move constructor.
         */
        IntrusivePointer(IntrusivePointer<T>&& rhs) noexcept :
            _p(std::exchange(rhs._p, nullptr))
        {
            // Reference count is unchanged
        }

        /**
         * @brief Default destructor.
         */
        ~IntrusivePointer() noexcept {
            this->releaseReference();
        }

        /**
         * @brief Assignment operator.
         */
        IntrusivePointer& operator=(const IntrusivePointer& rhs) noexcept {
            if (this->_p != rhs._p) {
                // addReference the new pointer before releaseReference'ing the old.
                T* pOld = this->_p;
                this->_p = rhs._p;
                addReference();

                if (pOld) {
                    pOld->releaseReference();
                }
            }

            return *this;
        }

        /**
         * @brief Assignment operator.
         */
        IntrusivePointer& operator=(T* p) noexcept {
            if (this->_p != p) {
                // addReference the new pointer before releaseReference'ing the old.
                T* pOld = this->_p;
                this->_p = p;
                addReference();

                if (pOld) {
                    pOld->releaseReference();
                }
            }

            return *this;
        }

        /**
         * @brief Dereferencing operator.
         */
        T& operator*() const noexcept {
            return *this->_p;
        }

        /**
         * @brief Arrow operator.
         */
        T* operator->() const noexcept {
            return this->_p;
        }

        /**
         * @brief Implicit conversion to `bool`, being `true` iff this is not the `nullptr`.
         */
        explicit operator bool() const noexcept {
            return this->_p != nullptr;
        }

        /**
         * @brief Returns the internal pointer.
         */
        T* get() const {
            return this->_p;
        }

        /**
         * @brief Returns `true` if two pointers are equal.
         */
        bool operator==(const IntrusivePointer<T>& rhs) const noexcept {
            return this->_p == rhs._p;
        }

        /**
         * @brief Returns `true` if two pointers are *not* equal.
         */
        bool operator!=(const IntrusivePointer<T>& rhs) const noexcept {
            return !(*this == rhs);
        }

        /**
         * @brief Returns `true` if the contents of this pointer is equal to the given pointer.
         */
        bool operator==(T* pRhs) const noexcept {
            return this->_p == pRhs;
        }

        /**
         * @brief Returns `true` if the contents of this pointer is *not* equal to the given pointer.
         */
        bool operator!=(T* pRhs) const noexcept {
            return !(*this == pRhs);
        }

    private:
        void addReference() noexcept {
            if (this->_p) {
                this->_p->addReference();
            }
        }

        void releaseReference() noexcept {
            if (this->_p) {
                this->_p->releaseReference();
            }
        }

        T* _p;
    };
}
