#pragma once

#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/Library.h>

#include <cstdint>

namespace CesiumJsonReader {
/**
 * @brief \ref IJsonHandler that does nothing but ignore the next value.
 *
 * Each `read...` call will return the current parent of this JsonHandler,
 * unless the value being read is an object or array, in which case it will
 * continue until the object or array is ended before returning.
 */
class CESIUMJSONREADER_API IgnoreValueJsonHandler : public IJsonHandler {
public:
  /**
   * @brief Resets the parent of this \ref IJsonHandler.
   */
  void reset(IJsonHandler* pParent) noexcept;

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

  /** @copydoc IJsonHandler::reportWarning */
  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

  /**
   * @brief Obtains the currently set parent of this handler, or `nullptr` if
   * `reset` hasn't yet been called.
   */
  IJsonHandler* parent() noexcept;

private:
  IJsonHandler* _pParent = nullptr;
  int32_t _depth = 0;
};
} // namespace CesiumJsonReader
