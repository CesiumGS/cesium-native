#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading bool values.
 */
class CESIUMJSONREADER_API BoolJsonHandler : public JsonHandler {
public:
  BoolJsonHandler() noexcept;
  /**
   * @brief Resets the parent \ref IJsonHandler of this handler, and the pointer
   * to its destination bool value.
   */
  void reset(IJsonHandler* pParent, bool* pBool);

  /** @copydoc IJsonHandler::readBool */
  virtual IJsonHandler* readBool(bool b) override;

private:
  bool* _pBool = nullptr;
};
} // namespace CesiumJsonReader
