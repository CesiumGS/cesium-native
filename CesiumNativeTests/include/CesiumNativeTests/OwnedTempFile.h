#pragma once

#include <gsl/span>

#include <filesystem>
#include <ios>

/**
 * Creates and holds on to a path for a temporary file on disk.
 * When the class is destructed, the file will be deleted if it still exists.
 */
class OwnedTempFile {
public:
  OwnedTempFile();
  OwnedTempFile(const gsl::span<const std::byte>& buffer);
  ~OwnedTempFile();

  const std::filesystem::path& getPath() const;

  void write(
      const gsl::span<const std::byte>& buffer,
      std::ios::openmode flags = std::ios::out | std::ios::binary |
                                 std::ios::trunc);

private:
  std::filesystem::path _filePath;
};
