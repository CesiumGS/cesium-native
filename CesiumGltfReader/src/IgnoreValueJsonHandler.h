#pragma once

#include "JsonHandler.h"
#include <cstdint>

namespace CesiumGltf {
    class IgnoreValueJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent);

        virtual JsonHandler* Null() override;
        virtual JsonHandler* Bool(bool /*b*/) override;
        virtual JsonHandler* Int(int /*i*/) override;
        virtual JsonHandler* Uint(unsigned /*i*/) override;
        virtual JsonHandler* Int64(int64_t /*i*/) override;
        virtual JsonHandler* Uint64(uint64_t /*i*/) override;
        virtual JsonHandler* Double(double /*d*/) override;
        virtual JsonHandler* RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) override;
        virtual JsonHandler* String(const char* /*str*/, size_t /*length*/, bool /*copy*/) override;
        virtual JsonHandler* StartObject() override;
        virtual JsonHandler* Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) override;
        virtual JsonHandler* EndObject(size_t /*memberCount*/) override;
        virtual JsonHandler* StartArray() override;
        virtual JsonHandler* EndArray(size_t /*elementCount*/) override;

    private:
        size_t _depth = 0;
    };
}
