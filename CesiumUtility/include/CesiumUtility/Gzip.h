#pragma once
#include <gsl/span>

#include <vector>

namespace CesiumUtility {

struct Gzip {
  /**
   * @brief Checks whether the data is gzipped.
   *
   * @param data The data.
   *
   * @returns Whether the data is gzipped
   */
  static bool isGzip(const gsl::span<const std::byte>& data);

  /**
   * @brief Gzips data.
   *
   * If successful, it will return true and the result will be in the
   * provided vector.
   *
   * @param data The data to gzip.
   * @param out The gzipped data.
   *
   * @returns True if successful, false otherwise.
   */
  static bool
  gzip(const gsl::span<const std::byte>& data, std::vector<std::byte>& out);

  /**
   * @brief Gunzips data.
   *
   * If successful, it will return true and the result will be in the
   * provided vector.
   *
   * @param data The data to gunzip.
   * @param out The gunzipped data.
   *
   * @returns True if successful, false otherwise.
   */
  static bool
  gunzip(const gsl::span<const std::byte>& data, std::vector<std::byte>& out);
};

} // namespace CesiumUtility