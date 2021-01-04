#pragma once

#include <cstdint>

namespace CesiumGltf {

    class JsonHandler {
    public:
        virtual JsonHandler* Null();
        virtual JsonHandler* Bool(bool /*b*/);
        virtual JsonHandler* Int(int /*i*/);
        virtual JsonHandler* Uint(unsigned /*i*/);
        virtual JsonHandler* Int64(int64_t /*i*/);
        virtual JsonHandler* Uint64(uint64_t /*i*/);
        virtual JsonHandler* Double(double /*d*/);
        virtual JsonHandler* RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/);
        virtual JsonHandler* String(const char* /*str*/, size_t /*length*/, bool /*copy*/);
        virtual JsonHandler* StartObject();
        virtual JsonHandler* Key(const char* /*str*/, size_t /*length*/, bool /*copy*/);
        virtual JsonHandler* EndObject(size_t /*memberCount*/);
        virtual JsonHandler* StartArray();
        virtual JsonHandler* EndArray(size_t /*elementCount*/);

        template <typename TAccessor, typename TProperty>
        JsonHandler* property(TAccessor& accessor, TProperty& value) {
            accessor.reset(this, &value);
            return &accessor;
        }

        JsonHandler* parent();

    protected:
        void reset(JsonHandler* pParent);

    private:
        JsonHandler* _pParent = nullptr;
    };

}
