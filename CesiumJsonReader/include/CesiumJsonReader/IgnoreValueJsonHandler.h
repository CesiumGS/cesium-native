#pragma once

#include "CesiumJsonReader/IJsonReader.h"
#include <cstdint>

namespace CesiumGltf {
class IgnoreValueJsonHandler : public IJsonReader {
public:
  void reset(IJsonReader* pParent);

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

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

  IJsonReader* parent();

private:
  IJsonReader* _pParent = nullptr;
  int32_t _depth = 0;
};
} // namespace CesiumGltf
