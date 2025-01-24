#include <CesiumNativeTests/OwnedTempFile.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <random>
#include <span>
#include <string>

constexpr size_t randFilenameLen = 8;
constexpr size_t randFilenameNumChars = 63;
static const char randFilenameChars[randFilenameNumChars] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

namespace {
std::string getTempFilename() {
  std::string str = "CesiumTest_";
  str.reserve(str.length() + randFilenameLen);

  std::uniform_int_distribution<size_t> dist(0, randFilenameNumChars - 1);
  std::random_device d;
  std::mt19937 gen(d());

  for (size_t i = 0; i < randFilenameLen; i++) {
    str += randFilenameChars[dist(gen)];
  }

  auto path = std::filesystem::temp_directory_path() / str;
  return path.string();
}
} // namespace

OwnedTempFile::OwnedTempFile() : _filePath(getTempFilename()) {}

OwnedTempFile::OwnedTempFile(const std::span<const std::byte>& buffer)
    : OwnedTempFile() {
  write(buffer);
}

OwnedTempFile::~OwnedTempFile() { std::filesystem::remove(this->_filePath); }

const std::filesystem::path& OwnedTempFile::getPath() const {
  return this->_filePath;
}

void OwnedTempFile::write(
    const std::span<const std::byte>& buffer,
    std::ios::openmode flags) {
  std::fstream stream(_filePath.string(), flags);
  REQUIRE(stream.good());
  stream.write(
      reinterpret_cast<const char*>(buffer.data()),
      static_cast<std::streamsize>(buffer.size()));
}
