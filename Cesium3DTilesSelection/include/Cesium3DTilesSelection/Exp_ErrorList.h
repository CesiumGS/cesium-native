#pragma once

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
struct ErrorList {
  void merge(const ErrorList& errorList);

  void merge(ErrorList&& errorList);

  template <typename ErrorStr> void emplace_error(ErrorStr&& error) {
    errors.emplace_back(std::forward<ErrorStr>(error));
  }

  template <typename WarningStr> void emplace_warning(WarningStr&& warning) {
    warnings.emplace_back(std::forward<WarningStr>(warning));
  }

  bool hasErrors() const noexcept;

  explicit operator bool() const noexcept;

  std::vector<std::string> errors;

  std::vector<std::string> warnings;
};
} // namespace Cesium3DTilesSelection
