#include <CesiumNativeTests/checkFilesEqual.h>
#include <CesiumNativeTests/readFile.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <vector>
#include <cstddef>

namespace CesiumNativeTests {
void checkFilesEqual(
    const std::filesystem::path& fileA,
    const std::filesystem::path& fileB) {
  const std::vector<std::byte>& bytes = readFile(fileA);
  const std::vector<std::byte>& bytes2 = readFile(fileB);

  REQUIRE(bytes.size() == bytes2.size());
  for (size_t i = 0; i < bytes.size(); i++) {
    CHECK(bytes[i] == bytes2[i]);
  }
}
} // namespace