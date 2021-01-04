#pragma once

#include "JsonHandler.h"
#include <string>

namespace CesiumGltf {
    class StringJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::string* pString);
        virtual JsonHandler* String(const char* str, size_t length, bool copy) override;

    private:
        std::string* _pString = nullptr;
    };
}
