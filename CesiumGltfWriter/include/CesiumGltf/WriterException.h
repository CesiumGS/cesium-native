#pragma once
#include <stdexcept>

class URIErroneouslyDefined : public std::runtime_error {
    public:
        URIErroneouslyDefined(const char* msg) : std::runtime_error(msg) {}
};