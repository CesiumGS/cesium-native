#pragma once

#include "JsonHandler.h"
#include <cassert>

namespace CesiumGltf {
    class DoubleJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, double* pDouble);

        virtual JsonHandler* Int(int i) override;
        virtual JsonHandler* Uint(unsigned i) override;
        virtual JsonHandler* Int64(int64_t i) override;
        virtual JsonHandler* Uint64(uint64_t i) override;
        virtual JsonHandler* Double(double d) override;

    private:
        double* _pDouble = nullptr;
    };
}
