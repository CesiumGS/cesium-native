#pragma once

#include <filesystem>
#include <vector>
#include <cstddef>

std::vector<std::byte> readFile(const std::filesystem::path& fileName);
