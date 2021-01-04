#pragma once

#include "JsonHandler.h"
#include "CesiumGltf/AlphaMode.h"

namespace CesiumGltf {
    class AlphaModeJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, AlphaMode* pEnum);
        virtual JsonHandler* String(const char* str, size_t length, bool copy) override;

    private:
        AlphaMode* _pEnum = nullptr;
    };
}
