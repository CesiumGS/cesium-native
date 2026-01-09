#pragma once

#include <filesystem>

namespace CesiumNativeTests {
/**
 * @brief Tests that the contents of `fileA` are equal to the contents of `fileB`.
 */
void checkFilesEqual(
    const std::filesystem::path& fileA,
    const std::filesystem::path& fileB);
} // namespace