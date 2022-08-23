#include <Cesium3DTilesSelection/ErrorList.h>

namespace Cesium3DTilesSelection {
void ErrorList::merge(const ErrorList& errorList) {
  errors.insert(errors.end(), errorList.errors.begin(), errorList.errors.end());
  warnings.insert(
      warnings.end(),
      errorList.warnings.begin(),
      errorList.warnings.end());
}

void ErrorList::merge(ErrorList&& errorList) {
  errors.reserve(errors.size() + errorList.errors.size());
  for (auto& error : errorList.errors) {
    errors.emplace_back(std::move(error));
  }

  warnings.reserve(warnings.size() + errorList.warnings.size());
  for (auto& warning : errorList.warnings) {
    warnings.emplace_back(std::move(warning));
  }
}

bool ErrorList::hasErrors() const noexcept { return !errors.empty(); }

ErrorList::operator bool() const noexcept { return hasErrors(); }
} // namespace Cesium3DTilesSelection
