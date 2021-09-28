#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <optional>

namespace CesiumJsonReader {
class CESIUMJSONREADER_API ObjectJsonHandler : public JsonHandler {
public:
  ObjectJsonHandler() noexcept : JsonHandler() {}

  virtual IJsonHandler* readObjectStart() override /* final */;
  virtual IJsonHandler* readObjectEnd() override /* final */;

protected:
  virtual IJsonHandler* StartSubObject() noexcept;
  virtual IJsonHandler* EndSubObject() noexcept;

  template <typename TAccessor, typename TProperty>
  IJsonHandler*
  property(const char* currentKey, TAccessor& accessor, TProperty& value) {
    this->_currentKey = currentKey;

    if constexpr (isOptional<TProperty>::value) {
      value.emplace();
      accessor.reset(this, &value.value());
    } else {
      accessor.reset(this, &value);
    }

    return &accessor;
  }

  const char* getCurrentKey() const noexcept;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

protected:
  void setCurrentKey(const char* key) noexcept;

private:
  template <typename T> struct isOptional {
    static constexpr bool value = false;
  };

  template <typename T> struct isOptional<std::optional<T>> {
    static constexpr bool value = true;
  };

  int32_t _depth = 0;
  const char* _currentKey = nullptr;
};
} // namespace CesiumJsonReader
