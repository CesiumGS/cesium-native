#pragma once

#include "IJsonHandler.h"
#include "Library.h"

#include <cstdint>

namespace CesiumJsonReader {
class CESIUMJSONREADER_API IgnoreValueJsonHandler : public IJsonHandler {
public:
  void reset(IJsonHandler* pParent) noexcept;

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

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

  IJsonHandler* parent() noexcept;

private:
  IJsonHandler* _pParent = nullptr;
  int32_t _depth = 0;
};
} // namespace CesiumJsonReader
