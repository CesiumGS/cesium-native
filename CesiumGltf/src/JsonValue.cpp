#include "CesiumGltf/JsonValue.h"

using namespace CesiumGltf;

double JsonValue::getNumber(double default) const {
    const double* p = std::get_if<Number>(&this->value);
    if (p) {
        return *p;
    } else {
        return default;
    }
}

bool JsonValue::getBool(bool default) const {
    const bool* p = std::get_if<Bool>(&this->value);
    if (p) {
        return *p;
    } else {
        return default;
    }
}

std::string JsonValue::getString(const std::string& default) const {
    const std::string* p = std::get_if<String>(&this->value);
    if (p) {
        return *p;
    } else {
        return default;
    }
}
