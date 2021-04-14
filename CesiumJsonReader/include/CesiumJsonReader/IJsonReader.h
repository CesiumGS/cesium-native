#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace CesiumJsonReader {
class IJsonReader {
public:
  virtual ~IJsonReader(){};
  virtual IJsonReader* readNull() = 0;
  virtual IJsonReader* readBool(bool b) = 0;
  virtual IJsonReader* readInt32(int32_t i) = 0;
  virtual IJsonReader* readUint32(uint32_t i) = 0;
  virtual IJsonReader* readInt64(int64_t i) = 0;
  virtual IJsonReader* readUint64(uint64_t i) = 0;
  virtual IJsonReader* readDouble(double d) = 0;
  virtual IJsonReader* readString(const std::string_view& str) = 0;
  virtual IJsonReader* readObjectStart() = 0;
  virtual IJsonReader* readObjectKey(const std::string_view& str) = 0;
  virtual IJsonReader* readObjectEnd() = 0;
  virtual IJsonReader* readArrayStart() = 0;
  virtual IJsonReader* readArrayEnd() = 0;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) = 0;
};
} // namespace CesiumJsonReader
