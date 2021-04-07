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
  virtual IJsonHandler* readInt32(int32_t i) override;
  virtual IJsonHandler* readUint32(uint32_t i) override;
  virtual IJsonHandler* readInt64(int64_t i) override;
  virtual IJsonHandler* readUint64(uint64_t i) override;
  virtual IJsonHandler* readDouble(double d) override;
  virtual IJsonHandler* readString(const std::string_view& str) override;
  virtual IJsonHandler* readObjectStart() override;
  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;
  virtual IJsonHandler* readObjectEnd() override;
  virtual IJsonHandler* readArrayStart() override;
  virtual IJsonHandler* readArrayEnd() override;

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
