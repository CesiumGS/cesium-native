#pragma once
#include <stdexcept>
namespace CesiumGltf {
class URIErroneouslyDefined : public std::runtime_error {
public:
  URIErroneouslyDefined(const char* msg) : std::runtime_error(msg) {}
};

class DataBufferSetButURINotSet : public std::runtime_error {
public:
  DataBufferSetButURINotSet(const char* msg) : std::runtime_error(msg) {}
};

class ByteLengthNotSet : public std::runtime_error {
public:
  ByteLengthNotSet(const char* msg) : std::runtime_error(msg) {}
};

class AmbiguiousDataSource : public std::runtime_error {
public:
  AmbiguiousDataSource(const char* msg) : std::runtime_error(msg) {}
};

class MissingDataSource : public std::runtime_error {
public:
  MissingDataSource(const char* msg) : std::runtime_error(msg) {}
};
} // namespace CesiumGltf