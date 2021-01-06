#pragma once

#include "JsonHandler.h"

namespace CesiumGltf {
    class ObjectJsonHandler : public JsonHandler {
    public:
        virtual IJsonHandler* StartObject() override final;
        virtual IJsonHandler* EndObject(size_t memberCount) override final;

    protected:
        virtual IJsonHandler* StartSubObject();
        virtual IJsonHandler* EndSubObject(size_t memberCount);

        template <typename TAccessor, typename TProperty>
        IJsonHandler* property(const char* currentKey, TAccessor& accessor, TProperty& value) {
            this->_currentKey = currentKey;
            accessor.reset(this, &value);
            return &accessor;
        }

        const char* getCurrentKey() const;

    private:
        int32_t _depth = 0;
        const char* _currentKey;
   };
}
