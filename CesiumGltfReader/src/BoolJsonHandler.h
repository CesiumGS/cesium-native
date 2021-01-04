#pragma once

#include "JsonHandler.h"

namespace CesiumGltf {
    class BoolJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, bool* pBool);

        virtual JsonHandler* Bool(bool b) override;

    private:
        bool* _pBool = nullptr;
    };
}
