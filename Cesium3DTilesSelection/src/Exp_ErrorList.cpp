#include <Cesium3DTilesSelection/Exp_ErrorList.h>

namespace Cesium3DTilesSelection {
void ErrorList::merge(const ErrorList& errorList) {
  errors.insert(errors.end(), errorList.errors.begin(), errorList.errors.end());
  warnings.insert(
      warnings.end(),
      errorList.warnings.begin(),
      errorList.warnings.end());
}

void ErrorList::merge(ErrorList&& errorList) {
  for (auto& error : errorList.errors) {
    errors.emplace_back(std::move(error));
  }

  for (auto& warning : errorList.warnings) {
    warnings.emplace_back(std::move(warning));
  }
}

bool ErrorList::hasErrors() const noexcept { return !errors.empty(); }

ErrorList::operator bool() const noexcept { return hasErrors(); }
} // namespace Cesium3DTilesSelection
