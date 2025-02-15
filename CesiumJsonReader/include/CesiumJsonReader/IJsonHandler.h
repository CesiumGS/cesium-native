#pragma once

#include "Library.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace CesiumJsonReader {
class CESIUMJSONREADER_API IJsonHandler {
public:
  virtual ~IJsonHandler(){};
  virtual IJsonHandler* readNull() = 0;
  virtual IJsonHandler* readBool(bool b) = 0;
  virtual IJsonHandler* readInt32(int32_t i) = 0;
  virtual IJsonHandler* readUint32(uint32_t i) = 0;
  virtual IJsonHandler* readInt64(int64_t i) = 0;
  virtual IJsonHandler* readUint64(uint64_t i) = 0;
  virtual IJsonHandler* readDouble(double d) = 0;
  virtual IJsonHandler* readString(const std::string_view& str) = 0;
  virtual IJsonHandler* readObjectStart() = 0;
  virtual IJsonHandler* readObjectKey(const std::string_view& str) = 0;
  virtual IJsonHandler* readObjectEnd() = 0;
  virtual IJsonHandler* readArrayStart() = 0;
  virtual IJsonHandler* readArrayEnd() = 0;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) = 0;
};
} // namespace CesiumJsonReader
