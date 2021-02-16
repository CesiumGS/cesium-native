#pragma once

#include "CesiumGltf/Library.h"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <map>
#include <variant>
#include <vector>

namespace CesiumGltf {

    /**
     * @brief A generic implementation of a value in a JSON structure.
     * 
     * Instances of this class are used to represent the common `extras` field
     * of glTF elements that extend the the {@link ExtensibleObject} class.
     */
    class CESIUMGLTF_API JsonValue final {
    public:

        /**
         * @brief The type to represent a `null` JSON value.
         */
        using Null = std::nullptr_t;

        /**
         * @brief The type to represent a `Number` JSON value.
         */
        using Number = double;

        /**
         * @brief The type to represent a `Bool` JSON value.
         */
        using Bool = bool;

        /**
         * @brief The type to represent a `String` JSON value.
         */
        using String = std::string;

        /**
         * @brief The type to represent an `Object` JSON value.
         */
        using Object = std::map<std::string, JsonValue>;

        /**
         * @brief The type to represent an `Array` JSON value.
         */
        using Array = std::vector<JsonValue>;

        /**
         * @brief Default constructor.
         */
        JsonValue() : value() {}

        /**
         * @brief Creates a `null` JSON value.
         */
        JsonValue(std::nullptr_t) : value(nullptr) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(double v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(int8_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(uint8_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(int16_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(uint16_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(int32_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(uint32_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(int64_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(uint64_t v) : JsonValue(static_cast<double>(v)) {}

        /**
         * @brief Creates a `Bool` JSON value.
         */
        JsonValue(bool v) : value(v) {}

        /**
         * @brief Creates a `String` JSON value.
         */
        JsonValue(const std::string& v) : value(v) {}

        /**
         * @brief Creates a `String` JSON value.
         */
        JsonValue(std::string&& v) : value(std::move(v)) {}

        /**
         * @brief Creates a `String` JSON value.
         */
        JsonValue(const char* v) : value(std::string(v)) {}

        /**
         * @brief Creates an `Object` JSON value with the given properties.
         */
        JsonValue(const std::map<std::string, JsonValue>& v) : value(v) {}

        /**
         * @brief Creates an `Object` JSON value with the given properties.
         */
        JsonValue(std::map<std::string, JsonValue>&& v) : value(std::move(v)) {}

        /**
         * @brief Creates an `Array` JSON value with the given elements.
         */
        JsonValue(const std::vector<JsonValue>& v) : value(v) {}

        /**
         * @brief Creates an `Array` JSON value with the given elements.
         */
        JsonValue(std::vector<JsonValue>&& v) : value(std::move(v)) {}

        /**
         * @brief Creates an JSON value from the given initializer list.
         */
        JsonValue(std::initializer_list<JsonValue> v) :
            value(std::vector<JsonValue>(v)) {}

        /**
         * @brief Creates an JSON value from the given initializer list.
         */
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

        /**
         * @brief Returns whether this value is a `null` value.
         */
        bool isNull() const;

        /**
         * @brief Returns whether this value is a `Number` value.
         */
        bool isNumber() const;

        /**
         * @brief Returns whether this value is a `Bool` value.
         */
        bool isBool() const;

        /**
         * @brief Returns whether this value is a `String` value.
         */
        bool isString() const;

        /**
         * @brief Returns whether this value is an `Object` value.
         */
        bool isObject() const;

        /**
         * @brief Returns whether this value is an `Array` value.
         */
        bool isArray() const;

        /** 
         * @brief The actual value. 
         * 
         * The type of the value may be queried with the `isNull`, `isNumber`, 
         * `isBool`, `isString`, `isObject`, and `isArray` functions.
         * 
         * The actual value may be obtained with the `getNumber`, `getBool`,
         * and `getString` functions for the respective types. For 
         * `Object` values, the properties may be accessed with the 
         * `getValueForKey` functions.
         */
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
