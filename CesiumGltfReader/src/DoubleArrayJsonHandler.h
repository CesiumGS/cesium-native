#pragma once

#include "JsonHandler.h"
#include <vector>

namespace CesiumGltf {
    class DoubleArrayJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::vector<double>* pArray);
        virtual JsonHandler* StartArray() override;
        virtual JsonHandler* EndArray(size_t count) override;
        virtual JsonHandler* Double(double d) override;

    private:
        std::vector<double>* _pArray = nullptr;
        bool _arrayIsOpen = false;
    };
}
