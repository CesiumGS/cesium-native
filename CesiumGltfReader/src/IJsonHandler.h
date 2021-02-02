#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumGltf {
    class IJsonHandler {
    public:
        virtual IJsonHandler* Null() = 0;
        virtual IJsonHandler* Bool(bool b) = 0;
        virtual IJsonHandler* Int(int i) = 0;
        virtual IJsonHandler* Uint(unsigned i) = 0;
        virtual IJsonHandler* Int64(int64_t i) = 0;
        virtual IJsonHandler* Uint64(uint64_t i) = 0;
        virtual IJsonHandler* Double(double d) = 0;
        virtual IJsonHandler* RawNumber(const char* str, size_t length, bool copy) = 0;
        virtual IJsonHandler* String(const char* str, size_t length, bool copy) = 0;
        virtual IJsonHandler* StartObject() = 0;
        virtual IJsonHandler* Key(const char* str, size_t length, bool copy) = 0;
        virtual IJsonHandler* EndObject(size_t memberCount) = 0;
        virtual IJsonHandler* StartArray() = 0;
        virtual IJsonHandler* EndArray(size_t elementCount) = 0;

        virtual void reportWarning(const std::string& warning, std::vector<std::string>&& context = std::vector<std::string>()) = 0;
    };
}
