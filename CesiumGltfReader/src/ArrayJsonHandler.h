#pragma once

#include "JsonHandler.h"
#include "DoubleJsonHandler.h"
#include <cassert>
#include <vector>

namespace CesiumGltf {
    template <typename T, typename THandler>
    class ArrayJsonHandler : public JsonHandler {
    public:
        void reset(IJsonHandler* pParent, std::vector<T>* pArray) {
            JsonHandler::reset(pParent);
            this->_pArray = pArray;
            this->_arrayIsOpen = false;
        }

        virtual IJsonHandler* StartArray() override {
            if (this->_arrayIsOpen) {
                return nullptr;
            }

            this->_arrayIsOpen = true;
            return this;
        }

        virtual IJsonHandler* EndArray(size_t) override {
            return this->parent();
        }

        virtual IJsonHandler* StartObject() override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            T& o = this->_pArray->emplace_back();
            this->_objectHandler.reset(this, &o);
            return this->_objectHandler.StartObject();
        }

        virtual void reportWarning(const std::string& warning, std::vector<std::string>&& context = std::vector<std::string>()) override {
            T* pObject = this->_objectHandler.getObject();
            int64_t index = pObject - this->_pArray->data();
            context.push_back(std::string("[") + std::to_string(index) + "]");
            this->parent()->reportWarning(warning, std::move(context));
        }

    private:
        std::vector<T>* _pArray = nullptr;
        bool _arrayIsOpen = false;
        THandler _objectHandler;
    };

    template <>
    class ArrayJsonHandler<double, DoubleJsonHandler> : public JsonHandler {
    public:
        void reset(IJsonHandler* pParent, std::vector<double>* pArray) {
            JsonHandler::reset(pParent);
            this->_pArray = pArray;
            this->_arrayIsOpen = false;
        }

        virtual IJsonHandler* StartArray() override {
            if (this->_arrayIsOpen) {
                return nullptr;
            }

            this->_arrayIsOpen = true;
            return this;
        }

        virtual IJsonHandler* EndArray(size_t) override {
            return this->parent();
        }

        virtual IJsonHandler* Int(int i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }
        
        virtual IJsonHandler* Uint(unsigned i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }

        virtual IJsonHandler* Int64(int64_t i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }

        virtual IJsonHandler* Uint64(uint64_t i) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(static_cast<double>(i));
            return this;
        }

        virtual IJsonHandler* Double(double d) override {
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
