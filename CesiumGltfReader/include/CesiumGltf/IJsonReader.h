#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumGltf {
class IJsonHandler {
public:
  virtual ~IJsonHandler(){};
  virtual IJsonHandler* readNull() = 0;
  virtual IJsonHandler* readBool(bool b) = 0;
  virtual IJsonHandler* readInt32(int i) = 0;
  virtual IJsonHandler* readUint32(unsigned i) = 0;
  virtual IJsonHandler* readInt64(int64_t i) = 0;
  virtual IJsonHandler* readUint64(uint64_t i) = 0;
  virtual IJsonHandler* readDouble(double d) = 0;
  virtual IJsonHandler*
  readString(const char* str, size_t length, bool copy) = 0;
  virtual IJsonHandler* readObjectStart() = 0;
  virtual IJsonHandler*
  readObjectKey(const char* str, size_t length, bool copy) = 0;
  virtual IJsonHandler* readObjectEnd(size_t memberCount) = 0;
  virtual IJsonHandler* readArrayStart() = 0;
  virtual IJsonHandler* readArrayEnd(size_t elementCount) = 0;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) = 0;
};
} // namespace CesiumGltf
