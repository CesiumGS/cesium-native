#pragma once

#include "CesiumJsonReader/JsonReader.h"
#include <optional>

namespace CesiumJsonReader {
class ObjectJsonHandler : public JsonReader {
public:
  ObjectJsonHandler() : JsonReader() {}

  virtual IJsonReader* readObjectStart() override /* final */;
  virtual IJsonReader* readObjectEnd() override /* final */;

protected:
  virtual IJsonReader* StartSubObject();
  virtual IJsonReader* EndSubObject();

  template <typename TAccessor, typename TProperty>
  IJsonReader*
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

  const char* getCurrentKey() const;

protected:
  void setCurrentKey(const char* key);

private:
  template <typename T> struct isOptional {
    static constexpr bool value = false;
  };

  template <typename T> struct isOptional<std::optional<T>> {
    static constexpr bool value = true;
  };

  int32_t _depth = 0;
  const char* _currentKey;
};
} // namespace CesiumJsonReader
