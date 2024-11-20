#pragma once
#include <span>
#include <vector>

namespace CesiumUtility {

/**
 * @brief Checks whether the data is gzipped.
 *
 * @param data The data.
 *
 * @returns Whether the data is gzipped
 */
bool isGzip(const std::span<const std::byte>& data);

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
bool gzip(const std::span<const std::byte>& data, std::vector<std::byte>& out);

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
bool gunzip(
    const std::span<const std::byte>& data,
    std::vector<std::byte>& out);

} // namespace CesiumUtility
