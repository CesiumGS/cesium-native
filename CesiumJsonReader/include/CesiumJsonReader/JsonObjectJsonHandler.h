#pragma once

#include <CesiumJsonReader/JsonHandler.h>
#include <CesiumJsonReader/Library.h>
#include <CesiumUtility/JsonValue.h>

namespace CesiumJsonReader {

/**
 * @brief \ref IJsonHandler for arbitrary \ref CesiumUtility::JsonValue
 * JsonValue objects.
 */
class CESIUMJSONREADER_API JsonObjectJsonHandler : public JsonHandler {
public:
  JsonObjectJsonHandler() noexcept;

  /**
   * @brief Resets this \ref IJsonHandler's parent handler, as well as its
   * destination pointer.
   */
  void reset(IJsonHandler* pParent, CesiumUtility::JsonValue* pValue);

  /** @copydoc IJsonHandler::readNull */
  virtual IJsonHandler* readNull() override;
  /** @copydoc IJsonHandler::readBool */
  virtual IJsonHandler* readBool(bool b) override;
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
  /** @copydoc IJsonHandler::readString */
  virtual IJsonHandler* readString(const std::string_view& str) override;
  /** @copydoc IJsonHandler::readObjectStart */
  virtual IJsonHandler* readObjectStart() override;
  /** @copydoc IJsonHandler::readObjectKey */
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;
  /** @copydoc IJsonHandler::readObjectEnd */
  virtual IJsonHandler* readObjectEnd() override;
  /** @copydoc IJsonHandler::readArrayStart */
  virtual IJsonHandler* readArrayStart() override;
  /** @copydoc IJsonHandler::readArrayEnd */
  virtual IJsonHandler* readArrayEnd() override;

private:
  IJsonHandler* doneElement();

  std::vector<CesiumUtility::JsonValue*> _stack;
  std::string_view _currentKey;
};

} // namespace CesiumJsonReader
