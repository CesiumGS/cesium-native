#pragma once

#include <CesiumJsonReader/IJsonHandler.h>
#include <CesiumJsonReader/IgnoreValueJsonHandler.h>
#include <CesiumJsonReader/Library.h>

#include <cstdint>
#include <string>

namespace CesiumJsonReader {
/**
 * @brief A dummy implementation of \ref IJsonHandler that will report a warning
 * and return its parent when any of its `read...` methods are called.
 *
 * This class can be used as a base for any specialized JsonHandlers, overriding
 * the methods required for the value you're handling and leaving the rest to
 * report warnings if called.
 */
class CESIUMJSONREADER_API JsonHandler : public IJsonHandler {
public:
  JsonHandler() noexcept;
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

protected:
  /**
   * @brief Resets the parent \ref IJsonHandler of this handler.
   */
  void reset(IJsonHandler* pParent);

  /**
   * @brief Obtains the parent \ref IJsonHandler of this handler.
   */
  IJsonHandler* parent();

  /**
   * @brief Ignore a single value and then return to the parent handler.
   */
  IJsonHandler* ignoreAndReturnToParent();

  /**
   * @brief Ignore a single value and the continue processing more tokens with
   * this handler.
   */
  IJsonHandler* ignoreAndContinue();

private:
  IJsonHandler* _pParent = nullptr;
  IgnoreValueJsonHandler _ignore;
};

} // namespace CesiumJsonReader
