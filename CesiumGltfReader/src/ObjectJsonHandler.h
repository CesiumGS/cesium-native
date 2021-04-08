#pragma once

#include "CesiumGltf/JsonReader.h"
#include "CesiumGltf/Reader.h"
#include <optional>

namespace CesiumGltf {
class ObjectJsonHandler : public JsonHandler {
public:
  ObjectJsonHandler(const JsonReaderContext& context) : JsonHandler(context) {}

  virtual IJsonHandler* readObjectStart() override /* final */;
  virtual IJsonHandler* readObjectEnd() override /* final */;

protected:
  virtual IJsonHandler* StartSubObject();
  virtual IJsonHandler* EndSubObject();

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
} // namespace CesiumGltf
