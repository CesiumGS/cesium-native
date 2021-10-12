#pragma once

#include "JsonHandler.h"
#include "Library.h"

namespace CesiumJsonWriter {
class CESIUMJSONWRITER_API BoolJsonHandler : public JsonHandler {
public:
  BoolJsonHandler() noexcept;
  void reset(IJsonHandler* pParent, bool* pBool);

  virtual IJsonHandler* writeBool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumJsonWriter
