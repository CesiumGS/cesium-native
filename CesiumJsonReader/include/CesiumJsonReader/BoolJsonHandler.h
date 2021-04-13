#pragma once

#include "CesiumJsonReader/JsonReader.h"

namespace CesiumGltf {
class BoolJsonHandler : public JsonReader {
public:
  BoolJsonHandler() noexcept;
  void reset(IJsonReader* pParent, bool* pBool);

  virtual IJsonReader* readBool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumGltf
