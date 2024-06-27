#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace CesiumUtility {

class BabyNames {
public:
  static const BabyNames& instance();

  BabyNames();

  template <typename T> std::string lookup(const T& o) const {
    std::hash<T> hasher;
    size_t value = hasher(o);
    size_t low = value & 0xFFFFFFFF;
    size_t high = value >> 32;
    return this->_names[low % this->_names.size()] + " " +
           this->_names[high % this->_names.size()];
  }

  template <typename T>
  void logOnName(const T& o, const std::string_view& name) const {
    if (this->lookup(o) == name) {
      SPDLOG_WARN("BabyName: {}", name);
    }
  }

private:
  std::vector<std::string> _names;
};

} // namespace CesiumUtility
