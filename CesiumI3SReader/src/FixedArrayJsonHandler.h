#pragma once

#include <CesiumJsonReader/JsonHandler.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace CesiumI3SReader {

// Reads a fixed-size JSON number array into std::array<T, N>.
template <typename T, size_t N>
class FixedArrayJsonHandler : public CesiumJsonReader::JsonHandler {
public:
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, std::array<T, N>* pArray) {
    JsonHandler::reset(pParent);
    _pArray = pArray;
    _index = 0;
    _arrayIsOpen = false;
  }

  CesiumJsonReader::IJsonHandler* readArrayStart() override {
    _index = 0;
    _arrayIsOpen = true;
    return this;
  }

  CesiumJsonReader::IJsonHandler* readArrayEnd() override {
    return this->parent();
  }

  CesiumJsonReader::IJsonHandler* readDouble(double d) override {
    if (_arrayIsOpen && _pArray && _index < N)
      (*_pArray)[_index++] = static_cast<T>(d);
    return this;
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

private:
  std::array<T, N>* _pArray = nullptr;
  size_t _index = 0;
  bool _arrayIsOpen = false;
};

} // namespace CesiumI3SReader
