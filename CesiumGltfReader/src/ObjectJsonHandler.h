#pragma once

#include "JsonHandler.h"
#include <optional>

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

            if constexpr (isOptional<TProperty>) {
                value.emplace();
                accessor.reset(this, &value.value());
            } else {
                accessor.reset(this, &value);
            }

            return &accessor;
        }

        const char* getCurrentKey() const;

    private:
        template <typename T>
        static constexpr bool isOptional = false;

        template <typename T>
        static constexpr bool isOptional<std::optional<T>> = true;

        int32_t _depth = 0;
        const char* _currentKey;
   };
}
