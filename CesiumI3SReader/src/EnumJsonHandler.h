#pragma once

#include <CesiumJsonReader/JsonHandler.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CesiumI3SReader {

// Forward-declared free function; specialized per-enum in EnumMappings.cpp.
template <typename E>
std::optional<E> enumFromString(const std::string_view& str);

/**
 * @brief Reads a single JSON string value into an enum of type `E`.
 *
 * Reports a warning if the string does not map to a known enumerator.
 *
 * @tparam E The enum type to deserialize into.
 */
template <typename E>
class EnumJsonHandler : public CesiumJsonReader::JsonHandler {
public:
  void reset(CesiumJsonReader::IJsonHandler* pParent, E* pEnum) {
    JsonHandler::reset(pParent);
    _pEnum = pEnum;
  }

  CesiumJsonReader::IJsonHandler*
  readString(const std::string_view& str) override {
    if (_pEnum) {
      auto result = enumFromString<E>(str);
      if (result) {
        *_pEnum = *result;
      } else {
        this->reportWarning(
            std::string("Unknown enum value: ") + std::string(str));
      }
    }
    return this->parent();
  }

private:
  E* _pEnum = nullptr;
};

/**
 * @brief Reads a JSON string array into a `std::vector<E>`.
 *
 * Each string element is mapped through `enumFromString<E>`. Unknown values
 * produce a warning and are skipped.
 *
 * @tparam E The enum type for each array element.
 */
template <typename E>
class EnumArrayJsonHandler : public CesiumJsonReader::JsonHandler {
public:
  void reset(CesiumJsonReader::IJsonHandler* pParent, std::vector<E>* pArray) {
    JsonHandler::reset(pParent);
    _pArray = pArray;
    _arrayIsOpen = false;
  }

  CesiumJsonReader::IJsonHandler* readArrayStart() override {
    _arrayIsOpen = true;
    if (_pArray)
      _pArray->clear();
    return this;
  }

  CesiumJsonReader::IJsonHandler* readArrayEnd() override {
    return this->parent();
  }

  CesiumJsonReader::IJsonHandler*
  readString(const std::string_view& str) override {
    if (!_arrayIsOpen || !_pArray)
      return this->parent();
    auto result = enumFromString<E>(str);
    if (result) {
      _pArray->emplace_back(*result);
    } else {
      this->reportWarning(
          std::string("Unknown enum value: ") + std::string(str));
    }
    return this;
  }

private:
  std::vector<E>* _pArray = nullptr;
  bool _arrayIsOpen = false;
};

} // namespace CesiumI3SReader
