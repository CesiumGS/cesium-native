#pragma once

#include "CesiumGltf/JsonReader.h"
#include <cassert>

namespace CesiumGltf {
class DoubleJsonHandler : public JsonHandler {
public:
  DoubleJsonHandler(const ReadModelOptions& options) noexcept;
  void reset(IJsonHandler* pParent, double* pDouble);

  virtual IJsonHandler* readInt32(int i) override;
  virtual IJsonHandler* readUint32(unsigned i) override;
  virtual IJsonHandler* readInt64(int64_t i) override;
  virtual IJsonHandler* readUint64(uint64_t i) override;
  virtual IJsonHandler* readDouble(double d) override;

private:
  double* _pDouble = nullptr;
};
} // namespace CesiumGltf
