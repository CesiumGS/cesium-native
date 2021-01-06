#pragma once

#include "JsonHandler.h"
#include "DoubleJsonHandler.h"
#include <cassert>
#include <vector>

namespace CesiumGltf {
    template <typename T, typename THandler>
    class ArrayJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::vector<T>* pArray) {
            JsonHandler::reset(pParent);
            this->_pArray = pArray;
            this->_arrayIsOpen = false;
        }

        virtual JsonHandler* StartArray() override {
            if (this->_arrayIsOpen) {
                return nullptr;
            }

            this->_arrayIsOpen = true;
            return this;
        }

        virtual JsonHandler* EndArray(size_t) override {
            return this->parent();
        }

        virtual JsonHandler* StartObject() override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            T& o = this->_pArray->emplace_back();
            this->_objectHandler.reset(this, &o);
            return this->_objectHandler.StartObject();
        }

    private:
        std::vector<T>* _pArray = nullptr;
        bool _arrayIsOpen = false;
        THandler _objectHandler;
    };

    template <>
    class ArrayJsonHandler<double, DoubleJsonHandler> : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::vector<double>* pArray) {
            JsonHandler::reset(pParent);
            this->_pArray = pArray;
            this->_arrayIsOpen = false;
        }

        virtual JsonHandler* StartArray() override {
            if (this->_arrayIsOpen) {
                return nullptr;
            }

            this->_arrayIsOpen = true;
            return this;
        }

        virtual JsonHandler* EndArray(size_t) override {
            return this->parent();
        }

        virtual JsonHandler* Int(int i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }
        
        virtual JsonHandler* Uint(unsigned i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }

        virtual JsonHandler* Int64(int64_t i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }

        virtual JsonHandler* Uint64(uint64_t i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }

        virtual JsonHandler* Double(double d) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(d);
            return this;
        }

    private:
        std::vector<double>* _pArray = nullptr;
        bool _arrayIsOpen = false;
    };
}
