#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <string>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading string values.
 */
class CESIUMJSONREADER_API StringJsonHandler : public JsonHandler {
public:
  StringJsonHandler() noexcept;
  /**
   * @brief Resets the parent \ref IJsonHandler of this handler, and its string
   * value.
   */
  void reset(IJsonHandler* pParent, std::string* pString);
  /**
   * @brief Obtains the string value of this \ref IJsonHandler, or nullptr if no
   * string has been read.
   */
  std::string* getObject() noexcept;
  /** @copydoc IJsonHandler::readString */
  virtual IJsonHandler* readString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumJsonReader
