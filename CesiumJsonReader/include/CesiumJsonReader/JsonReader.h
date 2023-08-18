#pragma once

#include "JsonHandler.h"
#include "Library.h"

#include <gsl/span>
#include <rapidjson/stream.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace rapidjson {
struct MemoryStream;
}

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
class CESIUMJSONREADER_API JsonReader {
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
    ReadJsonResult<typename T::ValueType> result;

    result.value.emplace();

    FinalJsonHandler finalHandler(result.warnings);
    handler.reset(&finalHandler, &result.value.value());

    JsonReader::internalRead(
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
  struct CesiumMemoryStream {
    typedef char Ch; // byte

    CesiumMemoryStream(Ch* src, size_t size)
        : src_(src), begin_(src), end_(src + size), size_(size) {}

    Ch Peek() const { return RAPIDJSON_UNLIKELY(src_ == end_) ? '\0' : *src_; }
    Ch Take() { return RAPIDJSON_UNLIKELY(src_ == end_) ? '\0' : *src_++; }
    size_t Tell() const { return static_cast<size_t>(src_ - begin_); }

    Ch* PutBegin() { return dst_ = src_; }
    void Put(Ch c) { RAPIDJSON_ASSERT(dst_ != 0); *dst_++ = c; }
    void Flush() {}
    size_t PutEnd(Ch* begin) { return static_cast<size_t>(dst_ - begin); }

    // For encoding detection only.
    const Ch* Peek4() const { return Tell() + 4 <= size_ ? src_ : 0; }

    Ch* src_;         //!< Current read position.
    const Ch* begin_; //!< Original head of the string.
    const Ch* end_;   //!< End of stream.
    size_t size_;     //!< Size of the stream.
    Ch* dst_;
  };

  class FinalJsonHandler : public JsonHandler {
  public:
    FinalJsonHandler(std::vector<std::string>& warnings);
    virtual void reportWarning(
        const std::string& warning,
        std::vector<std::string>&& context) override;
    // void setInputStream(rapidjson::MemoryStream* pInputStream) noexcept;
    void setInputStream(CesiumMemoryStream* pInputStream) noexcept;

  private:
    std::vector<std::string>& _warnings;
    CesiumMemoryStream* _pInputStream;
  };

  static void internalRead(
      const gsl::span<const std::byte>& data,
      IJsonHandler& handler,
      FinalJsonHandler& finalHandler,
      std::vector<std::string>& errors,
      std::vector<std::string>& warnings);
};

} // namespace CesiumJsonReader
