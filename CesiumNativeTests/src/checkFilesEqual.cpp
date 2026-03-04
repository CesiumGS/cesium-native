#include <CesiumNativeTests/checkFilesEqual.h>
#include <CesiumNativeTests/readFile.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <vector>

namespace CesiumNativeTests {
void checkFilesEqual(
    const std::filesystem::path& fileA,
    const std::filesystem::path& fileB) {
  const std::vector<std::byte>& bytes = readFile(fileA);
  const std::vector<std::byte>& bytes2 = readFile(fileB);

  REQUIRE(bytes == bytes2);
}
} // namespace CesiumNativeTests