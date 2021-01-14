#pragma once

#include <variant>
#include <vector>
#include <unordered_map>

namespace CesiumGltf {
    class JsonValue final {
    public:
        using Null = nullptr_t;
        using Number = double;
        using Bool = bool;
        using String = std::string;
        using Object = std::unordered_map<std::string, JsonValue>;
        using Array = std::vector<JsonValue>;

        JsonValue() : value() {}
        JsonValue(nullptr_t) : value(nullptr) {}
        JsonValue(double v) : value(v) {}
        JsonValue(bool v) : value(v) {}
        JsonValue(const std::string& v) : value(v) {}
        JsonValue(std::string&& v) : value(std::move(v)) {}
        JsonValue(const char* v) : value(std::string(v)) {}
        JsonValue(const std::unordered_map<std::string, JsonValue>& v) : value(v) {}
        JsonValue(std::unordered_map<std::string, JsonValue>&& v) : value(std::move(v)) {}
        JsonValue(const std::vector<JsonValue>& v) : value(v) {}
        JsonValue(std::vector<JsonValue>&& v) : value(std::move(v)) {}

        JsonValue(std::initializer_list<JsonValue> v) :
            value(std::vector<JsonValue>(v)) {}
        JsonValue(std::initializer_list<std::pair<const std::string, JsonValue>> v) :
            value(std::unordered_map<std::string, JsonValue>(v)) {}

        /**
         * @brief Gets the number from the value, or a default if the value does not contain a number.
         * 
         * @param default The default value to return if the value is not a number.
         * @return The number.
         */
        double getNumber(double default) const;

        /**
         * @brief Gets the bool from the value, or a default if the value does not contain a bool.
         * 
         * @param default The default value to return if the value is not a bool.
         * @return The bool.
         */
        bool getBool(bool default) const;

        /**
         * @brief Gets the string from the value, or a default if the value does not contain a string.
         * 
         * @param default The default value to return if the value is not a string.
         * @return The string.
         */
        std::string getString(const std::string& default) const;

        std::variant<
            Null,
            Number,
            Bool,
            String,
            Object,
            Array
        > value;
    };
}
