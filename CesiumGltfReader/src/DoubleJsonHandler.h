#pragma once

#include "CesiumGltf/JsonHandler.h"
#include <cassert>

namespace CesiumGltf {
class DoubleJsonHandler : public JsonHandler {
public:
  DoubleJsonHandler(const ReadModelOptions& options) noexcept;
  void reset(IJsonHandler* pParent, double* pDouble);

  virtual IJsonHandler* Int(int i) override;
  virtual IJsonHandler* Uint(unsigned i) override;
  virtual IJsonHandler* Int64(int64_t i) override;
  virtual IJsonHandler* Uint64(uint64_t i) override;
  virtual IJsonHandler* Double(double d) override;

private:
  double* _pDouble = nullptr;
};
} // namespace CesiumGltf
