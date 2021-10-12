#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <string>

namespace CesiumJsonWriter {
class CESIUMJSONWRITER_API StringJsonHandler : public JsonHandler {
public:
  StringJsonHandler() noexcept;
  void reset(IJsonHandler* pParent, std::string* pString);
  std::string* getObject() noexcept;
  virtual IJsonHandler* writeString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumJsonWriter
