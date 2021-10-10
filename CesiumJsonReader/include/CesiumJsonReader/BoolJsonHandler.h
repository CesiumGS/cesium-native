#pragma once

#include "JsonHandler.h"
#include "Library.h"

namespace CesiumJsonReader {
class CESIUMJSONREADER_API BoolJsonHandler : public JsonHandler {
public:
  BoolJsonHandler() noexcept;
  void reset(IJsonHandler* pParent, bool* pBool);

  virtual IJsonHandler* readBool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumJsonReader
