#pragma once

#include <utility>
#include <type_traits>

namespace CesiumUtility {
template <typename ExistFunction> class ScopeGuard {
public:
  template <
      typename ExistFunctionArg,
      typename std::enable_if_t<!std::is_same_v<
          std::remove_reference_t<std::remove_const_t<ExistFunctionArg>>,
          ScopeGuard<ExistFunction>>, int> = 0>
  explicit ScopeGuard(ExistFunctionArg&& existFunction)
      : _callExistFuncOnDestruct{true},
        _existFunc{std::forward<ExistFunctionArg>(existFunction)} {}

  ScopeGuard(const ScopeGuard& rhs) = delete;

  ScopeGuard(ScopeGuard&& rhs) noexcept
      : _callExistFuncOnDestruct{rhs._callExistFuncOnDestruct},
        _existFunc{std::move(rhs._existFunc)} {
    rhs.release();
  }

  ScopeGuard& operator=(const ScopeGuard& rhs) = delete;

  ScopeGuard& operator=(ScopeGuard&& rhs) noexcept {
    if (&rhs != this) {
      _callExistFuncOnDestruct = rhs._callExistFuncOnDestruct;
      _existFunc = std::move(rhs._existFunc);
      rhs.release();
    }

    return *this;
  }

  ~ScopeGuard() noexcept {
    if (_callExistFuncOnDestruct) {
      _existFunc();
    }
  }

  void release() noexcept { _callExistFuncOnDestruct = false; }

private:
  bool _callExistFuncOnDestruct;
  ExistFunction _existFunc;
};

template <typename ExistFunction>
ScopeGuard(ExistFunction)->ScopeGuard<ExistFunction>;
} // namespace CesiumUtility
