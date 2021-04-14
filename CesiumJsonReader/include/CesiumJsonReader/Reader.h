#pragma once

#include "CesiumJsonReader/IJsonHandler.h"
#include "CesiumJsonReader/Library.h"
#include <cstddef>
#include <gsl/span>
#include <optional>
#include <string>
#include <vector>

namespace CesiumJsonReader {

/**
 * @brief The result of {@link Reader::readJson}.
 */
template <typename T> struct ReadJsonResult {
  /**
   * @brief The value read from the JSON, or `std::nullopt` on error.
   */
  std::optional<T> value;

  /**
   * @brief Errors that occurred while reading.
   */
  std::vector<std::string> errors;

  /**
   * @brief Warnings that occurred while reading.
   */
  std::vector<std::string> warnings;
};

/**
 * @brief Reads JSON.
 */
class CESIUMJSONREADER_API Reader {
public:
  /**
   * @brief Reads JSON from a byte buffer.
   *
   * @param data The buffer from which to read JSON.
   * @param handler The handler to receive the top-level JSON object.
   * @return The result of reading the JSON.
   */
  template <typename T>
  static ReadJsonResult<typename T::ValueType>
  readJson(const gsl::span<const std::byte>& data, T& handler) {
    ReadJsonResult<T::ValueType> result;

    result.value.emplace();

    FinalHandler finalHandler(result.warnings);
    handler.reset(&finalHandler, &result.value.value());

    Reader::internalRead(data, handler, result.errors, result.warnings);

    if (!result.errors.empty()) {
      result.value.reset();
    }

    return result;
  }

private:
  static void internalRead(
      const gsl::span<const std::byte>& data,
      IJsonHandler& handler,
      std::vector<std::string>& errors,
      std::vector<std::string>& warnings);
};

} // namespace CesiumJsonReader
