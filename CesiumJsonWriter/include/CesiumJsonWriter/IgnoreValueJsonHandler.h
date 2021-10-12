#pragma once

#include "IJsonHandler.h"
#include "Library.h"

#include <cstdint>

namespace CesiumJsonWriter {
class CESIUMJSONWRITER_API IgnoreValueJsonHandler : public IJsonHandler {
public:
  void reset(IJsonHandler* pParent) noexcept;

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

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

  IJsonHandler* parent() noexcept;

private:
  IJsonHandler* _pParent = nullptr;
  int32_t _depth = 0;
};
} // namespace CesiumJsonWriter
