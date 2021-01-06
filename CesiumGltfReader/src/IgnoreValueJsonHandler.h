#pragma once

#include "IJsonHandler.h"
#include <cstdint>

namespace CesiumGltf {
    class IgnoreValueJsonHandler : public IJsonHandler {
    public:
        void reset(IJsonHandler* pParent);

        virtual IJsonHandler* Null() override;
        virtual IJsonHandler* Bool(bool b) override;
        virtual IJsonHandler* Int(int i) override;
        virtual IJsonHandler* Uint(unsigned i) override;
        virtual IJsonHandler* Int64(int64_t i) override;
        virtual IJsonHandler* Uint64(uint64_t i) override;
        virtual IJsonHandler* Double(double d) override;
        virtual IJsonHandler* RawNumber(const char* str, size_t length, bool copy) override;
        virtual IJsonHandler* String(const char* str, size_t length, bool copy) override;
        virtual IJsonHandler* StartObject() override;
        virtual IJsonHandler* Key(const char* str, size_t length, bool copy) override;
        virtual IJsonHandler* EndObject(size_t memberCount) override;
        virtual IJsonHandler* StartArray() override;
        virtual IJsonHandler* EndArray(size_t elementCount) override;

        virtual void reportWarning(const std::string& warning, std::vector<std::string>&& context = std::vector<std::string>()) override;

        IJsonHandler* parent();

    private:
        IJsonHandler* _pParent = nullptr;
        int32_t _depth = 0;
    };
}
