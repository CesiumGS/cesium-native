#pragma once

#include "JsonHandler.h"
#include <CesiumGltf/Reader.h>
#include <string>

namespace CesiumGltf {
class StringJsonHandler : public JsonHandler {
public:
  StringJsonHandler(ReadModelOptions options) noexcept;
  void reset(IJsonHandler* pParent, std::string* pString);
  std::string* getObject();
  virtual IJsonHandler*
  String(const char* str, size_t length, bool copy) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumGltf
