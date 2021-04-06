#pragma once

#include "CesiumGltf/IJsonReader.h"
#include "CesiumGltf/ReadModelOptions.h"
#include "IgnoreValueJsonHandler.h"
#include <cstdint>
#include <string>

namespace CesiumGltf {
class JsonHandler : public IJsonHandler {
public:
  JsonHandler(const ReadModelOptions& options) noexcept;
  virtual IJsonHandler* readNull() override;
  virtual IJsonHandler* readBool(bool b) override;
  virtual IJsonHandler* readInt32(int i) override;
  virtual IJsonHandler* readUint32(unsigned i) override;
  virtual IJsonHandler* readInt64(int64_t i) override;
  virtual IJsonHandler* readUint64(uint64_t i) override;
  virtual IJsonHandler* readDouble(double d) override;
  virtual IJsonHandler*
  readString(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler* readObjectStart() override;
  virtual IJsonHandler*
  readObjectKey(const char* str, size_t length, bool copy) override;
  virtual IJsonHandler* readObjectEnd(size_t memberCount) override;
  virtual IJsonHandler* readArrayStart() override;
  virtual IJsonHandler* readArrayEnd(size_t elementCount) override;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

protected:
  void reset(IJsonHandler* pParent);

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

protected:
  const ReadModelOptions& _options;

private:
  IJsonHandler* _pParent = nullptr;
  IgnoreValueJsonHandler _ignore;
};

} // namespace CesiumGltf
