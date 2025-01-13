#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>

#include <string>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading string values.
 */
class CESIUMJSONREADER_API StringJsonHandler : public JsonHandler {
public:
  StringJsonHandler() noexcept;
  /**
   * @brief Resets the parent \ref IJsonHandler of this handler, and the pointer
   * to its destination string value.
   */
  void reset(IJsonHandler* pParent, std::string* pString);
  /**
   * @brief Obtains the pointer to the current destination string value of this
   * handler.
   */
  std::string* getObject() noexcept;
  /** @copydoc IJsonHandler::readString */
  virtual IJsonHandler* readString(const std::string_view& str) override;

private:
  std::string* _pString = nullptr;
};
} // namespace CesiumJsonReader
