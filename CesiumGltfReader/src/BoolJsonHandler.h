#pragma once

#include "CesiumGltf/JsonReader.h"
#include "CesiumGltf/Reader.h"

namespace CesiumGltf {
class BoolJsonHandler : public JsonReader {
public:
  BoolJsonHandler(const ReaderContext& context) noexcept;
  void reset(IJsonReader* pParent, bool* pBool);

  virtual IJsonReader* readBool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumGltf
