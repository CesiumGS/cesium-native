#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler for reading double values.
 */
class CESIUMJSONREADER_API DoubleJsonHandler : public JsonHandler {
public:
  DoubleJsonHandler() noexcept;
  /**
   * @brief Resets the parent \ref IJsonHandler of this handler, and the pointer
   * to its destination double value.
   */
  void reset(IJsonHandler* pParent, double* pDouble);

  /** @copydoc IJsonHandler::readInt32 */
  virtual IJsonHandler* readInt32(int32_t i) override;
  /** @copydoc IJsonHandler::readUint32 */
  virtual IJsonHandler* readUint32(uint32_t i) override;
  /** @copydoc IJsonHandler::readInt64 */
  virtual IJsonHandler* readInt64(int64_t i) override;
  /** @copydoc IJsonHandler::readUint64 */
  virtual IJsonHandler* readUint64(uint64_t i) override;
  /** @copydoc IJsonHandler::readDouble */
  virtual IJsonHandler* readDouble(double d) override;

private:
  double* _pDouble = nullptr;
};
} // namespace CesiumJsonReader
