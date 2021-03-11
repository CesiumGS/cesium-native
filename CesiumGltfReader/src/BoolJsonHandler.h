#pragma once

#include "JsonHandler.h"
#include <CesiumGltf/Reader.h>

namespace CesiumGltf {
class BoolJsonHandler : public JsonHandler {
public:
  BoolJsonHandler(ReadModelOptions options) noexcept;
  void reset(IJsonHandler* pParent, bool* pBool);

  virtual IJsonHandler* Bool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumGltf
