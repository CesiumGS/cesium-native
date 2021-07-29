#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

std::vector<std::byte> readFile(const std::filesystem::path& fileName);
