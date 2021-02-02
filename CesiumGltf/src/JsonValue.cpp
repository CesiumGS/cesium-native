#include "CesiumGltf/JsonValue.h"

using namespace CesiumGltf;

double JsonValue::getNumber(double defaultValue) const {
    const double* p = std::get_if<Number>(&this->value);
    if (p) {
        return *p;
    } else {
        return defaultValue;
    }
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

bool JsonValue::isNull() const {
    return std::holds_alternative<Null>(this->value);
}

bool JsonValue::isNumber() const {
    return std::holds_alternative<Number>(this->value);
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
