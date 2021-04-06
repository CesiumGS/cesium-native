#pragma once

#include "CesiumGltf/JsonHandler.h"
#include "CesiumGltf/JsonValue.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {

class JsonObjectJsonHandler : public JsonHandler {
public:
  JsonObjectJsonHandler(const ReadModelOptions& options) noexcept;
  void reset(IJsonHandler* pParent, JsonValue* pValue);

  virtual IJsonHandler* Null() override;
  virtual IJsonHandler* Bool(bool b) override;
  virtual IJsonHandler* Int(int i) override;
  virtual IJsonHandler* Uint(unsigned i) override;
  virtual IJsonHandler* Int64(int64_t i) override;
  virtual IJsonHandler* Uint64(uint64_t i) override;
  virtual IJsonHandler* Double(double d) override;
  virtual IJsonHandler*
  RawNumber(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler*
  String(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler* StartObject() override;
  virtual IJsonHandler* Key(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler* EndObject(size_t memberCount) override;
  virtual IJsonHandler* StartArray() override;
  virtual IJsonHandler* EndArray(size_t elementCount) override;

private:
  IJsonHandler* doneElement();

  std::vector<JsonValue*> _stack;
  const char* _currentKey;
};

} // namespace CesiumGltf
