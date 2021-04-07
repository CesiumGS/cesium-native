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
  virtual IJsonHandler* readInt32(int32_t i) override;
  virtual IJsonHandler* readUint32(uint32_t i) override;
  virtual IJsonHandler* readInt64(int64_t i) override;
  virtual IJsonHandler* readUint64(uint64_t i) override;
  virtual IJsonHandler* readDouble(double d) override;
  virtual IJsonHandler* readString(const std::string_view& str) override;
  virtual IJsonHandler* readObjectStart() override;
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;
  virtual IJsonHandler* readObjectEnd() override;
  virtual IJsonHandler* readArrayStart() override;
  virtual IJsonHandler* readArrayEnd() override;

private:
  IJsonHandler* doneElement();

  std::vector<JsonValue*> _stack;
  std::string_view _currentKey;
};

} // namespace CesiumGltf
