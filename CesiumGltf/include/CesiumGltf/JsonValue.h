#pragma once

#include "CesiumGltf/Library.h"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <map>
#include <variant>
#include <vector>

namespace CesiumGltf {
    class CESIUMGLTF_API JsonValue final {
    public:
        using Null = std::nullptr_t;
        using Number = double;
        using Bool = bool;
        using String = std::string;
        using Object = std::map<std::string, JsonValue>;
        using Array = std::vector<JsonValue>;

        JsonValue() : value() {}
        JsonValue(std::nullptr_t) : value(nullptr) {}
        JsonValue(double v) : value(v) {}
        JsonValue(int8_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(uint8_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(int16_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(uint16_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(int32_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(uint32_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(int64_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(uint64_t v) : JsonValue(static_cast<double>(v)) {}
        JsonValue(bool v) : value(v) {}
        JsonValue(const std::string& v) : value(v) {}
        JsonValue(std::string&& v) : value(std::move(v)) {}
        JsonValue(const char* v) : value(std::string(v)) {}
        JsonValue(const std::map<std::string, JsonValue>& v) : value(v) {}
        JsonValue(std::map<std::string, JsonValue>&& v) : value(std::move(v)) {}
        JsonValue(const std::vector<JsonValue>& v) : value(v) {}
        JsonValue(std::vector<JsonValue>&& v) : value(std::move(v)) {}

        JsonValue(std::initializer_list<JsonValue> v) :
            value(std::vector<JsonValue>(v)) {}
        JsonValue(std::initializer_list<std::pair<const std::string, JsonValue>> v) :
            value(std::map<std::string, JsonValue>(v)) {}

        /**
         * @brief Gets the number from the value, or a default if the value does not contain a number.
         * 
         * @param defaultValue The default value to return if the value is not a number.
         * @return The number.
         */
        double getNumber(double defaultValue) const;

        /**
         * @brief Gets the bool from the value, or a default if the value does not contain a bool.
         * 
         * @param defaultValue The default value to return if the value is not a bool.
         * @return The bool.
         */
        bool getBool(bool defaultValue) const;

        /**
         * @brief Gets the string from the value, or a default if the value does not contain a string.
         * 
         * @param defaultValue The default value to return if the value is not a string.
         * @return The string.
         */
        std::string getString(const std::string& defaultValue) const;

        /**
         * @brief Gets a typed value corresponding to the given key in the object represented by this instance.
         * 
         * If this instance is not a {@link JsonValue::Object}, returns `nullptr`. If the key does not exist in
         * this object, returns `nullptr`. If the named value does not have the type T, returns nullptr.
         * 
         * @tparam T The expected type of the value.
         * @param key The key for which to retrieve the value from this object.
         * @return A pointer to the requested value, or nullptr if the value cannot be obtained as requested.
         */
        template <typename T>
        const T* getValueForKey(const std::string& key) const {
            const JsonValue* pValue = this->getValueForKey(key);
            if (!pValue) {
                return nullptr;
            }

            return std::get_if<T>(&pValue->value);
        }

        /**
         * @brief Gets a typed value corresponding to the given key in the object represented by this instance.
         * 
         * If this instance is not a {@link JsonValue::Object}, returns `nullptr`. If the key does not exist in
         * this object, returns `nullptr`. If the named value does not have the type T, returns nullptr.
         * 
         * @tparam T The expected type of the value.
         * @param key The key for which to retrieve the value from this object.
         * @return A pointer to the requested value, or nullptr if the value cannot be obtained as requested.
         */
        template <typename T>
        T* getValueForKey(const std::string& key) {
            JsonValue* pValue = this->getValueForKey(key);
            if (!pValue) {
                return nullptr;
            }

            return std::get_if<T>(&pValue->value);
        }

        /**
         * @brief Gets the value corresponding to the given key in the object represented by this instance.
         * 
         * If this instance is not a {@link JsonValue::Object}, returns `nullptr`. If the key does not exist in
         * this object, returns `nullptr`. If the named value does not have the type T, returns nullptr.
         * 
         * @param key The key for which to retrieve the value from this object.
         * @return A pointer to the requested value, or nullptr if the value cannot be obtained as requested.
         */
        const JsonValue* getValueForKey(const std::string& key) const;

        /**
         * @brief Gets the value corresponding to the given key in the object represented by this instance.
         * 
         * If this instance is not a {@link JsonValue::Object}, returns `nullptr`. If the key does not exist in
         * this object, returns `nullptr`. If the named value does not have the type T, returns nullptr.
         * 
         * @param key The key for which to retrieve the value from this object.
         * @return A pointer to the requested value, or nullptr if the value cannot be obtained as requested.
         */
        JsonValue* getValueForKey(const std::string& key);

        bool isNull() const;
        bool isNumber() const;
        bool isBool() const;
        bool isString() const;
        bool isObject() const;
        bool isArray() const;

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
