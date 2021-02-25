#pragma once

#include "CesiumGltf/Library.h"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <map>
#include <variant>
#include <vector>
#include <optional>

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
        JsonValue(std::int8_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::uint8_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::int16_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::uint16_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::int32_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::uint32_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::int64_t v) : value(v) {}

        /**
         * @brief Creates a `Number` JSON value.
         */
        JsonValue(std::uint64_t v) : value(v) {}

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
        double getDoubleOrDefault(double defaultValue) const;

        /**
         * @brief Gets the number from the value, or a default if the value does not contain
         *        an unsigned integer.
         * 
         * @remark Automaticaly promotes uint8_t / uint16_t / uint32_t to uint64_t.
         *         Use getUIntX() if this is undesirable.
         * @return The number, or std::nullopt if value does not hold a uint
         */
        [[nodiscard]] std::optional<std::uint64_t> getUInt() const noexcept;

        /**
         * @brief Gets the number from the value, or a default if the value does not contain
         *        a signed integer.
         * 
         * @remark Automaticaly promotes int8_t / int16_t / int32_t to int64_t.
         *         Use getIntX() if this is undesirable.
         * @return The number, or std::nullopt if value does not hold a uint
         */
        [[nodiscard]] std::optional<std::int64_t> getInt() const noexcept;

        [[nodiscard]] std::optional<std::uint8_t> getUInt8() const noexcept;
        [[nodiscard]] std::optional<std::uint16_t> getUInt16() const noexcept;
        [[nodiscard]] std::optional<std::uint32_t> getUInt32() const noexcept;
        [[nodiscard]] std::optional<std::uint64_t> getUInt64() const noexcept;
        [[nodiscard]] std::optional<std::int8_t> getInt8() const noexcept;
        [[nodiscard]] std::optional<std::int16_t> getInt16() const noexcept;
        [[nodiscard]] std::optional<std::int32_t> getInt32() const noexcept;
        [[nodiscard]] std::optional<std::int64_t> getInt64() const noexcept;

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
        
        [[nodiscard]] std::optional<double> getValueForKeyAsDouble(const std::string& key) const noexcept;
        [[nodiscard]] std::optional<std::int32_t> getValueForKeyAsInt32(const std::string& key) const noexcept;
        [[nodiscard]] std::optional<std::size_t> getValueForKeyAsSizeT(const std::string& key) const noexcept;

        /**
         * @brief Returns whether this value is a `null` value.
         */
        bool isNull() const;

        /**
         * @brief Returns whether this value is a `double` value.
         */
        [[nodiscard]] bool isDouble() const noexcept;
        
        /**
         * @brief Returns whether this value is a `float` value.
         */
        [[nodiscard]] bool isFloat() const noexcept;

        /**
         * @brief Returns whether this value is a signed or unsigned
         *        integral value (8,16,32,64 bits supported)
         */
        [[nodiscard]] bool isInt() const noexcept;

        /**
         * @brief Returns whether this value is a signed
         *        integral value (8,16,32,64 bits supported).
         */
        [[nodiscard]] bool isSignedInt() const noexcept;

        /**
         * @brief Returns whether this value is a unsigned
         *        integral value (8,16,32,64 bits supported).
         */
        [[nodiscard]] bool isUnsignedInt() const noexcept;

        /**
         * @brief Returns whether this value is a `std::uint8_t` value.
         */
        [[nodiscard]] bool isUInt8() const noexcept;

        /**
         * @brief Returns whether this value is a `std::uint16_t` value.
         */
        [[nodiscard]] bool isUInt16() const noexcept;

        /**
         * @brief Returns whether this value is a `std::uint32_t` value.
         */
        [[nodiscard]] bool isUInt32() const noexcept;

        /**
         * @brief Returns whether this value is a `std::uint64_t` value.
         */
        [[nodiscard]] bool isUInt64() const noexcept;

        /**
         * @brief Returns whether this value is a `std::int8_t` value.
         */
        [[nodiscard]] bool isInt8() const noexcept;

        /**
         * @brief Returns whether this value is a `std::int16_t` value.
         */
        [[nodiscard]] bool isInt16() const noexcept;

        /**
         * @brief Returns whether this value is a `std::int32_t` value.
         */
        [[nodiscard]] bool isInt32() const noexcept;

        /**
         * @brief Returns whether this value is a `std::int64_t` value.
         */
        [[nodiscard]] bool isInt64() const noexcept;


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
         * The type of the value may be queried with the `isNull`, `isDouble`, 
         * `isBool`, `isString`, `isObject`, and `isArray` functions.
         * 
         * The actual value may be obtained with the `getNumber`, `getBool`,
         * and `getString` functions for the respective types. For 
         * `Object` values, the properties may be accessed with the 
         * `getValueForKey` functions.
         */
        std::variant<
            Null,
            double,
            float,
            std::uint8_t,
            std::uint16_t,
            std::uint32_t,
            std::uint64_t,
            std::int8_t,
            std::int16_t,
            std::int32_t,
            std::int64_t,
            Bool,
            String,
            Object,
            Array
        > value;
    };
}
