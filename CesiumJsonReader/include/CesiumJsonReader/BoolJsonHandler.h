#pragma once

#include "CesiumJsonReader/JsonReader.h"
#include "CesiumJsonReader/Library.h"

namespace CesiumJsonReader {
class CESIUMJSONREADER_API BoolJsonHandler : public JsonReader {
public:
  BoolJsonHandler() noexcept;
  void reset(IJsonReader* pParent, bool* pBool);

  virtual IJsonReader* readBool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumJsonReader
