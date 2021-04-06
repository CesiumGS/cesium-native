#pragma once

#include "CesiumGltf/IJsonReader.h"
#include <cstdint>

namespace CesiumGltf {
class IgnoreValueJsonHandler : public IJsonHandler {
public:
  void reset(IJsonHandler* pParent);

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

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

  IJsonHandler* parent();

private:
  IJsonHandler* _pParent = nullptr;
  int32_t _depth = 0;
};
} // namespace CesiumGltf
