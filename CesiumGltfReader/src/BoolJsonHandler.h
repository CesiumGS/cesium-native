#pragma once

#include "JsonHandler.h"

namespace CesiumGltf {
    class BoolJsonHandler : public JsonHandler {
    public:
        void reset(IJsonHandler* pParent, bool* pBool);

        virtual IJsonHandler* Bool(bool b) override;

    private:
        bool* _pBool = nullptr;
    };
}
