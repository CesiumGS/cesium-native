#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonWriter {

class CESIUMJSONWRITER_API JsonObjectJsonHandler : public JsonHandler {
public:
  JsonObjectJsonHandler() noexcept;

  void reset(IJsonHandler* pParent, CesiumUtility::JsonValue* pValue);

  virtual IJsonHandler* writeNull() override;
  virtual IJsonHandler* writeBool(bool b) override;
  virtual IJsonHandler* writeInt32(int32_t i) override;
  virtual IJsonHandler* writeUint32(uint32_t i) override;
  virtual IJsonHandler* writeInt64(int64_t i) override;
  virtual IJsonHandler* writeUint64(uint64_t i) override;
  virtual IJsonHandler* writeDouble(double d) override;
  virtual IJsonHandler* writeString(const std::string_view& str) override;
  virtual IJsonHandler* writeObjectStart() override;
  virtual IJsonHandler* writeObjectKey(const std::string_view& str) override;
  virtual IJsonHandler* writeObjectEnd() override;
  virtual IJsonHandler* writeArrayStart() override;
  virtual IJsonHandler* writeArrayEnd() override;

private:
  IJsonHandler* doneElement();

  std::vector<CesiumUtility::JsonValue*> _stack;
  std::string_view _currentKey;
};

} // namespace CesiumJsonWriter
