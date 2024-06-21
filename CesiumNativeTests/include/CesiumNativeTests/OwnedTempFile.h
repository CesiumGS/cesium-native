#pragma once

#include <catch2/catch.hpp>
#include <gsl/span>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

/**
 * Creates and holds on to a path for a temporary file on disk.
 * When the class is destructed, the file will be deleted if it still exists.
 */
class OwnedTempFile {
public:
  OwnedTempFile();
  OwnedTempFile(const gsl::span<const std::byte>& buffer);
  ~OwnedTempFile() {
    std::remove(reinterpret_cast<const char*>(_filePath.c_str()));
  }

  const std::filesystem::path& getPath() const { return _filePath; }

  void write(
      const gsl::span<const std::byte>& buffer,
      std::ios::fmtflags flags = std::ios::out | std::ios::binary |
                                 std::ios::trunc);

private:
  std::filesystem::path _filePath;
};
