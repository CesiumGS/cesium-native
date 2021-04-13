#pragma once

#include "CesiumJsonReader/IJsonReader.h"
#include "CesiumJsonReader/IgnoreValueJsonHandler.h"
#include <cstdint>
#include <string>

namespace CesiumGltf {
class JsonReader : public IJsonReader {
public:
  JsonReader() noexcept;
  virtual IJsonReader* readNull() override;
  virtual IJsonReader* readBool(bool b) override;
  virtual IJsonReader* readInt32(int32_t i) override;
  virtual IJsonReader* readUint32(uint32_t i) override;
  virtual IJsonReader* readInt64(int64_t i) override;
  virtual IJsonReader* readUint64(uint64_t i) override;
  virtual IJsonReader* readDouble(double d) override;
  virtual IJsonReader* readString(const std::string_view& str) override;
  virtual IJsonReader* readObjectStart() override;
  virtual IJsonReader* readObjectKey(const std::string_view& str) override;
  virtual IJsonReader* readObjectEnd() override;
  virtual IJsonReader* readArrayStart() override;
  virtual IJsonReader* readArrayEnd() override;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context = std::vector<std::string>()) override;

protected:
  void reset(IJsonReader* pParent);

  IJsonReader* parent();

  /**
   * @brief Ignore a single value and then return to the parent handler.
   */
  IJsonReader* ignoreAndReturnToParent();

  /**
   * @brief Ignore a single value and the continue processing more tokens with
   * this handler.
   */
  IJsonReader* ignoreAndContinue();

private:
  IJsonReader* _pParent = nullptr;
  IgnoreValueJsonHandler _ignore;
};

} // namespace CesiumGltf
