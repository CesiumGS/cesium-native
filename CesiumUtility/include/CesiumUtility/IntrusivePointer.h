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
        IntrusivePointer(T* p = nullptr) noexcept :
            _p(p)
        {
            this->addReference();
        }

        IntrusivePointer(const IntrusivePointer<T>& rhs) noexcept :
            _p(rhs._p)
        {
            this->addReference();
        }

        IntrusivePointer(IntrusivePointer<T>&& rhs) noexcept :
            _p(std::exchange(rhs._p, nullptr))
        {
            // Reference count is unchanged
        }

        ~IntrusivePointer() noexcept {
            this->releaseReference();
        }

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

        T& operator*() const noexcept {
            return *this->_p;
        }

        T* operator->() const noexcept {
            return this->_p;
        }

        explicit operator bool() const noexcept {
            return this->_p != nullptr;
        }

        T* get() const {
            return this->_p;
        }

        bool operator==(const IntrusivePointer<T>& rhs) const noexcept {
            return this->_p == rhs._p;
        }

        bool operator!=(const IntrusivePointer<T>& rhs) const noexcept {
            return !(*this == rhs);
        }

        bool operator==(T* pRhs) const noexcept {
            return this->_p == pRhs;
        }

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
