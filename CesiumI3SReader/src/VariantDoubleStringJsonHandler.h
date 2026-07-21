#pragma once

#include <CesiumJsonReader/JsonHandler.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace CesiumI3SReader {

// Reads a JSON string or number into std::variant<double, std::string>.
class VariantDoubleStringJsonHandler : public CesiumJsonReader::JsonHandler {
public:
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      std::variant<double, std::string>* pVariant) {
    JsonHandler::reset(pParent);
    _pVariant = pVariant;
  }

  CesiumJsonReader::IJsonHandler* readDouble(double d) override {
    if (_pVariant)
      *_pVariant = d;
    return this->parent();
  }
  CesiumJsonReader::IJsonHandler* readInt32(int32_t i) override {
    return readDouble(static_cast<double>(i));
  }
  CesiumJsonReader::IJsonHandler* readUint32(uint32_t i) override {
    return readDouble(static_cast<double>(i));
  }
  CesiumJsonReader::IJsonHandler* readInt64(int64_t i) override {
    return readDouble(static_cast<double>(i));
  }
  CesiumJsonReader::IJsonHandler* readUint64(uint64_t i) override {
    return readDouble(static_cast<double>(i));
  }
  CesiumJsonReader::IJsonHandler*
  readString(const std::string_view& str) override {
    if (_pVariant)
      *_pVariant = std::string(str);
    return this->parent();
  }

private:
  std::variant<double, std::string>* _pVariant = nullptr;
};

} // namespace CesiumI3SReader
