#pragma once

#include "CesiumJsonReader/JsonReader.h"
#include <string>

namespace CesiumJsonReader {
class StringJsonHandler : public JsonReader {
public:
  StringJsonHandler() noexcept;
  void reset(IJsonReader* pParent, std::string* pString);
  std::string* getObject();
  virtual IJsonReader* readString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumJsonReader
