#pragma once

#include "JsonHandler.h"

#include <variant>

namespace CesiumJsonReader {
template <typename... JsonHandlers, typename... Objects>
class VariantJsonHandler : public JsonHandler {
public:
  VariantJsonHandler() noexcept;

void reset(IJsonHandler* pParentHandler, std::variant<Objects...>* objects);

protected:
private:
  std::tuple<JsonHandlers...> _jsonHandlers;
};
} // namespace CesiumJsonReader
