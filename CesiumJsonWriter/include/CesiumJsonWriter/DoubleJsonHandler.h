#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <cassert>

namespace CesiumJsonWriter {
class CESIUMJSONWRITER_API DoubleJsonHandler : public JsonHandler {
public:
  DoubleJsonHandler() noexcept;
  void reset(IJsonHandler* pParent, double* pDouble);

  virtual IJsonHandler* writeInt32(int32_t i) override;
  virtual IJsonHandler* writeUint32(uint32_t i) override;
  virtual IJsonHandler* writeInt64(int64_t i) override;
  virtual IJsonHandler* writeUint64(uint64_t i) override;
  virtual IJsonHandler* writeDouble(double d) override;

private:
  double* _pDouble = nullptr;
};
} // namespace CesiumJsonWriter
