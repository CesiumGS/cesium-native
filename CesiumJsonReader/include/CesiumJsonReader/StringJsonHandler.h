#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <string>

namespace CesiumJsonReader {
class CESIUMJSONREADER_API StringJsonHandler : public JsonHandler {
public:
  StringJsonHandler() noexcept;
  void reset(IJsonHandler* pParent, std::string* pString);
  std::string* getObject() noexcept;
  virtual IJsonHandler* readString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumJsonReader
