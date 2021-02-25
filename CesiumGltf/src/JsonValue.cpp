#include "CesiumGltf/JsonValue.h"

using namespace CesiumGltf;

double JsonValue::getDoubleOrDefault(double defaultValue) const {
    const double* p = std::get_if<double>(&this->value);
    if (p) {
        return *p;
    } else {
        return defaultValue;
    }
}

std::optional<std::uint64_t> JsonValue::getUInt() const noexcept {
    auto* u8 = std::get_if<std::uint8_t>(&this->value);
    if (u8) {
        return static_cast<std::uint64_t>(*u8);
    }

    auto* u16 = std::get_if<std::uint16_t>(&this->value);
    if (u16) {
        return static_cast<std::uint64_t>(*u16);
    }

    auto* u32 = std::get_if<std::uint32_t>(&this->value);
    if (u32) {
        return static_cast<std::uint32_t>(*u32);
    }

    auto* u64 = std::get_if<std::uint64_t>(&this->value);
    if (u64) {
        return static_cast<std::uint64_t>(*u64);
    }

    return std::nullopt;
}

std::optional<std::int64_t> JsonValue::getInt() const noexcept {
    auto* i8 = std::get_if<std::int8_t>(&this->value);
    if (i8) {
        return static_cast<std::int64_t>(*i8);
    }

    auto* i16 = std::get_if<std::int16_t>(&this->value);
    if (i16) {
        return static_cast<std::int64_t>(*i16);
    }

    auto* i32 = std::get_if<std::int32_t>(&this->value);
    if (i32) {
        return static_cast<std::int32_t>(*i32);
    }

    auto* i64 = std::get_if<std::int64_t>(&this->value);
    if (i64) {
        return static_cast<std::int64_t>(*i64);
    }

    return std::nullopt;
}

std::optional<std::uint8_t> JsonValue::getUInt8() const noexcept {
    auto* u8 = std::get_if<std::uint8_t>(&this->value);
    if (u8) {
        return *u8;
    }

    return std::nullopt;
}
std::optional<std::uint16_t> JsonValue::getUInt16() const noexcept {
    auto* u16 = std::get_if<std::uint16_t>(&this->value);
    if (u16) {
        return *u16;
    }

    return std::nullopt;
}
std::optional<std::uint32_t> JsonValue::getUInt32() const noexcept {
    auto* u32 = std::get_if<std::uint32_t>(&this->value);
    if (u32) {
        return *u32;
    }

    return std::nullopt;
}
std::optional<std::uint64_t> JsonValue::getUInt64() const noexcept {
    auto* u64 = std::get_if<std::uint64_t>(&this->value);
    if (u64) {
        return *u64;
    }

    return std::nullopt;
}
std::optional<std::int8_t> JsonValue::getInt8() const noexcept {
    auto* i8 = std::get_if<std::int8_t>(&this->value);
    if (i8) {
        return *i8;
    }

    return std::nullopt;
}
std::optional<std::int16_t> JsonValue::getInt16() const noexcept {
    auto* i16 = std::get_if<std::int16_t>(&this->value);
    if (i16) {
        return *i16;
    }

    return std::nullopt;
}
std::optional<std::int32_t> JsonValue::getInt32() const noexcept {
    auto* i32 = std::get_if<std::int32_t>(&this->value);
    if (i32) {
        return *i32;
    }

    return std::nullopt;
}
std::optional<std::int64_t> JsonValue::getInt64() const noexcept {
    auto* i64 = std::get_if<std::int64_t>(&this->value);
    if (i64) {
        return *i64;
    }

    return std::nullopt;
}

bool JsonValue::getBool(bool defaultValue) const {
    const bool* p = std::get_if<Bool>(&this->value);
    if (p) {
        return *p;
    } else {
        return defaultValue;
    }
}

std::string JsonValue::getString(const std::string& defaultValue) const {
    const std::string* p = std::get_if<String>(&this->value);
    if (p) {
        return *p;
    } else {
        return defaultValue;
    }
}

const JsonValue* JsonValue::getValueForKey(const std::string& key) const {
    const Object* pObject = std::get_if<Object>(&this->value);
    if (!pObject) {
        return nullptr;
    }

    auto it = pObject->find(key);
    if (it == pObject->end()) {
        return nullptr;
    }

    return &it->second;
}

JsonValue* JsonValue::getValueForKey(const std::string& key) {
    Object* pObject = std::get_if<Object>(&this->value);
    if (!pObject) {
        return nullptr;
    }

    auto it = pObject->find(key);
    if (it == pObject->end()) {
        return nullptr;
    }

    return &it->second;
}

std::optional<double> JsonValue::getValueForKeyAsDouble(const std::string& key) const noexcept {
    const CesiumGltf::JsonValue* v = getValueForKey(key);
    if (v) {
        const auto* number = std::get_if<double>(&(v->value));
        if (number) {
            return *number;
        }
    }

    return std::nullopt;
}

std::optional<std::int32_t> JsonValue::getValueForKeyAsInt32(const std::string& key) const noexcept {
    const CesiumGltf::JsonValue* v = getValueForKey(key);
    if (v) {
        const auto* number = std::get_if<double>(&(v->value));
        if (number) {
            return static_cast<std::int32_t>(*number);
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> JsonValue::getValueForKeyAsSizeT(const std::string& key) const noexcept {
    const CesiumGltf::JsonValue* v = getValueForKey(key);
    if (v) {
        const auto* number = std::get_if<double>(&(v->value));
        if (number) {
            return static_cast<size_t>(*number);
        }
    }

    return std::nullopt;
}

bool JsonValue::isNull() const {
    return std::holds_alternative<Null>(this->value);
}

[[nodiscard]] bool JsonValue::isDouble() const noexcept {
    return std::holds_alternative<double>(this->value);
}

[[nodiscard]] bool JsonValue::isFloat() const noexcept {
    return std::holds_alternative<float>(this->value);
}

[[nodiscard]] bool JsonValue::isInt() const noexcept {
    return isSignedInt() || isUnsignedInt();
}

[[nodiscard]] bool JsonValue::isSignedInt() const noexcept {
    return isInt8() || isInt16() || isInt32() || isInt64();
}

[[nodiscard]] bool JsonValue::isUnsignedInt() const noexcept {
    return isUInt8() || isUInt16() || isUInt32() || isUInt64();
}

[[nodiscard]] bool JsonValue::isUInt8() const noexcept {
    return std::holds_alternative<std::uint8_t>(this->value);
}

[[nodiscard]] bool JsonValue::isUInt16() const noexcept {
    return std::holds_alternative<std::uint16_t>(this->value);
}

[[nodiscard]] bool JsonValue::isUInt32() const noexcept {
    return std::holds_alternative<std::uint32_t>(this->value);
}

[[nodiscard]] bool JsonValue::isUInt64() const noexcept {
    return std::holds_alternative<std::uint64_t>(this->value);
}

[[nodiscard]] bool JsonValue::isInt8() const noexcept {
    return std::holds_alternative<std::int8_t>(this->value);
}

[[nodiscard]] bool JsonValue::isInt16() const noexcept {
    return std::holds_alternative<std::int16_t>(this->value);
}

[[nodiscard]] bool JsonValue::isInt32() const noexcept {
    return std::holds_alternative<std::int32_t>(this->value);
}

[[nodiscard]] bool JsonValue::isInt64() const noexcept {
    return std::holds_alternative<std::int64_t>(this->value);
}

bool JsonValue::isBool() const {
    return std::holds_alternative<Bool>(this->value);
}

bool JsonValue::isString() const {
    return std::holds_alternative<String>(this->value);
}

bool JsonValue::isObject() const {
    return std::holds_alternative<Object>(this->value);
}

bool JsonValue::isArray() const {
    return std::holds_alternative<Array>(this->value);
}
