#pragma once

#include "CesiumGltf/JsonReader.h"
#include "CesiumGltf/Reader.h"
#include <string>

namespace CesiumGltf {
class StringJsonHandler : public JsonReader {
public:
  StringJsonHandler(const ReaderContext& context) noexcept;
  void reset(IJsonReader* pParent, std::string* pString);
  std::string* getObject();
  virtual IJsonReader* readString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumGltf
