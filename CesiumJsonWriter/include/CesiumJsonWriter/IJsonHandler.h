#pragma once

#include "Library.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace CesiumJsonWriter {
class CESIUMJSONWRITER_API IJsonHandler {
public:
  virtual ~IJsonHandler(){};
  virtual IJsonHandler* writeNull() = 0;
  virtual IJsonHandler* writeBool(bool b) = 0;
  virtual IJsonHandler* writeInt32(int32_t i) = 0;
  virtual IJsonHandler* writeUint32(uint32_t i) = 0;
  virtual IJsonHandler* writeInt64(int64_t i) = 0;
  virtual IJsonHandler* writeUint64(uint64_t i) = 0;
  virtual IJsonHandler* writeDouble(double d) = 0;
  virtual IJsonHandler* writeString(const std::string_view& str) = 0;
  virtual IJsonHandler* writeObjectStart() = 0;
  virtual IJsonHandler* writeObjectKey(const std::string_view& str) = 0;
  virtual IJsonHandler* writeObjectEnd() = 0;
  virtual IJsonHandler* writeArrayStart() = 0;
  virtual IJsonHandler* writeArrayEnd() = 0;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) = 0;
};
} // namespace CesiumJsonWriter
