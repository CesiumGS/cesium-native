#pragma once

#include "CesiumJsonReader/JsonReader.h"
#include "CesiumJsonReader/Library.h"
#include "CesiumUtility/JsonValue.h"

namespace CesiumJsonReader {

class CESIUMJSONREADER_API JsonObjectJsonHandler : public JsonReader {
public:
  JsonObjectJsonHandler() noexcept;

  void reset(IJsonReader* pParent, CesiumUtility::JsonValue* pValue);

  virtual IJsonReader* readNull() override;
  virtual IJsonReader* readBool(bool b) override;
  virtual IJsonReader* readInt32(int32_t i) override;
  virtual IJsonReader* readUint32(uint32_t i) override;
  virtual IJsonReader* readInt64(int64_t i) override;
  virtual IJsonReader* readUint64(uint64_t i) override;
  virtual IJsonReader* readDouble(double d) override;
  virtual IJsonReader* readString(const std::string_view& str) override;
  virtual IJsonReader* readObjectStart() override;
  virtual IJsonReader* readObjectKey(const std::string_view& str) override;
  virtual IJsonReader* readObjectEnd() override;
  virtual IJsonReader* readArrayStart() override;
  virtual IJsonReader* readArrayEnd() override;

private:
  IJsonReader* doneElement();

  std::vector<CesiumUtility::JsonValue*> _stack;
  std::string_view _currentKey;
};

} // namespace CesiumJsonReader
