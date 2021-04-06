#pragma once

#include "CesiumGltf/JsonReader.h"
#include "CesiumGltf/JsonValue.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {

class JsonObjectJsonHandler : public JsonHandler {
public:
  JsonObjectJsonHandler(const ReadModelOptions& options) noexcept;
  void reset(IJsonHandler* pParent, JsonValue* pValue);

  virtual IJsonHandler* readNull() override;
  virtual IJsonHandler* readBool(bool b) override;
  virtual IJsonHandler* readInt32(int i) override;
  virtual IJsonHandler* readUint32(unsigned i) override;
  virtual IJsonHandler* readInt64(int64_t i) override;
  virtual IJsonHandler* readUint64(uint64_t i) override;
  virtual IJsonHandler* readDouble(double d) override;
  virtual IJsonHandler*
  readString(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler* readObjectStart() override;
  virtual IJsonHandler*
  readObjectKey(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler* readObjectEnd(size_t memberCount) override;
  virtual IJsonHandler* readArrayStart() override;
  virtual IJsonHandler* readArrayEnd(size_t elementCount) override;

private:
  IJsonHandler* doneElement();

  std::vector<JsonValue*> _stack;
  const char* _currentKey;
};

} // namespace CesiumGltf
