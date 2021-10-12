#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <gsl/span>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace rapidjson {
struct MemoryStream;
}

namespace CesiumJsonWriter {

/**
 * @brief The result of {@link Writer::writeJson}.
 */
template <typename T> struct WriteJsonResult {
  /**
   * @brief The value write from the JSON, or `std::nullopt` on error.
   */
  std::optional<T> value;

  /**
   * @brief Errors that occurred while writing.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings that occurred while writing.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Writes JSON.
 */
class CESIUMJSONWRITER_API JsonWriter {
public:
  /**
   * @brief Writes JSON from a byte buffer.
   *
   * @param data The buffer from which to write JSON.
   * @param handler The handler to receive the top-level JSON object.
   * @return The result of writing the JSON.
   */
  template <typename T>
  static WriteJsonResult<typename T::ValueType>
  writeJson(const gsl::span<const std::byte>& data, T& handler) {
    WriteJsonResult<typename T::ValueType> result;

    result.value.emplace();

    FinalJsonHandler finalHandler(result.warnings);
    handler.reset(&finalHandler, &result.value.value());

    JsonWriter::internalWrite(
        data,
        handler,
        finalHandler,
        result.errors,
        result.warnings);

    if (!result.errors.empty()) {
      result.value.reset();
    }

    return result;
  }

private:
  class FinalJsonHandler : public JsonHandler {
  public:
    FinalJsonHandler(std::vector<std::string>& warnings);
    virtual void reportWarning(
        const std::string& warning,
        std::vector<std::string>&& context) override;
    void setInputStream(rapidjson::MemoryStream* pInputStream) noexcept;

  private:
    std::vector<std::string>& _warnings;
    rapidjson::MemoryStream* _pInputStream;
  };

  static void internalWrite(
      const gsl::span<const std::byte>& data,
      IJsonHandler& handler,
      FinalJsonHandler& finalHandler,
      std::vector<std::string>& errors,
      std::vector<std::string>& warnings);
};

} // namespace CesiumJsonWriter
