#pragma once

#include "CesiumJsonReader/JsonHandler.h"
#include "CesiumJsonReader/Library.h"
#include <optional>

namespace CesiumJsonReader {

namespace Impl {
template <typename TAccessor, typename TOwner, typename TValue>
struct ResetAccessor {
  static void go(TAccessor& accessor, TOwner& owner, TValue& value) {
    accessor.reset(&owner, &value);
  }
};

template <typename TAccessor, typename TOwner, typename TOptionalValue>
struct ResetAccessor<TAccessor, TOwner, std::optional<TOptionalValue>> {
  static void
  go(TAccessor& accessor, TOwner& owner, std::optional<TOptionalValue>& value) {
    value.emplace();
    accessor.reset(&owner, &value.value());
  }
};
} // namespace Impl

class CESIUMJSONREADER_API ObjectJsonHandler : public JsonHandler {
public:
  ObjectJsonHandler() : JsonHandler() {}

  virtual IJsonHandler* readObjectStart() override /* final */;
  virtual IJsonHandler* readObjectEnd() override /* final */;

protected:
  virtual IJsonHandler* StartSubObject();
  virtual IJsonHandler* EndSubObject();

  template <typename TAccessor, typename TProperty>
  IJsonHandler*
  property(const char* currentKey, TAccessor& accessor, TProperty& value) {
    this->_currentKey = currentKey;
    Impl::ResetAccessor<TAccessor, ObjectJsonHandler, TProperty>::go(
        accessor,
        *this,
        value);
    return &accessor;
  }

  const char* getCurrentKey() const;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

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
