#pragma once

#include "CesiumGltf/JsonReader.h"
#include "CesiumGltf/Reader.h"
#include <string>

namespace CesiumGltf {
class StringJsonHandler : public JsonHandler {
public:
  StringJsonHandler(const JsonReaderContext& context) noexcept;
  void reset(IJsonHandler* pParent, std::string* pString);
  std::string* getObject();
  virtual IJsonHandler* readString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumGltf
