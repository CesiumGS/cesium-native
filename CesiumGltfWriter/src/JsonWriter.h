#pragma once
#include "JsonWriter.h"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include <vector>

namespace CesiumGltf {
    struct JsonWriter {
        rapidjson::StringBuffer _stringBuffer;
        std::optional<rapidjson::Writer<rapidjson::StringBuffer>> writer;

    public:
        explicit JsonWriter();

        // rapidjson methods
        bool Null();
        bool Bool(bool b);
        bool Int(int i);
        bool Uint(unsigned int i);
        bool Uint64(std::uint64_t i);
        bool Int64(std::int64_t i);
        bool Double(double d);
        bool RawNumber(const char* str, unsigned int length, bool copy);
        bool Key(std::string_view string);
        bool String(std::string_view string);
        bool StartObject();
        bool EndObject();
        bool StartArray();
        bool EndArray();

        // Primitive overloads
        void Primitive(std::int32_t value);
        void Primitive(std::uint32_t value);
        void Primitive(std::int64_t value);
        void Primitive(std::uint64_t value);
        void Primitive(float value);
        void Primitive(double value);
        void Primitive(std::nullptr_t value);

        // Integral
        void KeyPrimitive(std::string_view keyName, std::int32_t value);
        void KeyPrimitive(std::string_view keyName, std::uint32_t value);
        void KeyPrimitive(std::string_view keyName, std::int64_t value);
        void KeyPrimitive(std::string_view keyName, std::uint64_t value);

        // Floating Point
        void KeyPrimitive(std::string_view keyName, float value);
        void KeyPrimitive(std::string_view keyName, double value);

        // Null
        void KeyPrimitive(std::string_view keyName, std::nullptr_t value);

        // Array / Objects
        void KeyArray(
            std::string_view keyName,
            std::function<void(void)> insideArray);

        template <typename T>
        inline void
        KeyArray(std::string_view keyName, const std::vector<T>& vector) {
            Key(keyName);
            writer->StartArray();
            for (const auto& v : vector) {
                Primitive(v);
            }
            writer->EndArray();
        }

        template <typename T, std::size_t Size>
        inline void
        KeyArray(std::string_view keyName, const std::array<T, Size>& array) {
            Key(keyName);
            writer->StartArray();
            for (const auto& v : array) {
                Primitive(v);
            }
            writer->EndArray();
        }

        void KeyObject(
            std::string_view keyName,
            std::function<void(void)> insideObject);

        std::string_view toString();
    };
}