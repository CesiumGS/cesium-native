#pragma once

#include "CesiumGltf/JsonReader.h"
#include <cassert>

namespace CesiumGltf {
class DoubleJsonHandler : public JsonReader {
public:
  DoubleJsonHandler(const ReaderContext& context) noexcept;
  void reset(IJsonReader* pParent, double* pDouble);

  virtual IJsonReader* readInt32(int32_t i) override;
  virtual IJsonReader* readUint32(uint32_t i) override;
  virtual IJsonReader* readInt64(int64_t i) override;
  virtual IJsonReader* readUint64(uint64_t i) override;
  virtual IJsonReader* readDouble(double d) override;

private:
  double* _pDouble = nullptr;
};
} // namespace CesiumGltf
