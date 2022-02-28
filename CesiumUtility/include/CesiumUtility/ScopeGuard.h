#pragma once

#include <utility>
#include <type_traits>

namespace CesiumUtility {
template <typename ExitFunction> class ScopeGuard {
public:
  template <
      typename ExitFunctionArg,
      typename std::enable_if_t<!std::is_same_v<
          std::remove_reference_t<std::remove_const_t<ExitFunctionArg>>,
          ScopeGuard<ExitFunction>>, int> = 0>
  explicit ScopeGuard(ExitFunctionArg&& _exitFunc)
      : _callExitFuncOnDestruct{true},
        _exitFunc{std::forward<ExitFunctionArg>(_exitFunc)} {}

  ScopeGuard(const ScopeGuard& rhs) = delete;

  ScopeGuard(ScopeGuard&& rhs) noexcept
      : _callExitFuncOnDestruct{rhs._callExitFuncOnDestruct},
        _exitFunc{std::move(rhs._exitFunc)} {
    rhs.Release();
  }

  ScopeGuard& operator=(const ScopeGuard& rhs) = delete;

  ScopeGuard& operator=(ScopeGuard&& rhs) noexcept {
    if (&rhs != this) {
      _callExitFuncOnDestruct = rhs._callExitFuncOnDestruct;
      _exitFunc = std::move(rhs._exitFunc);
      rhs.Release();
    }

    return *this;
  }

  ~ScopeGuard() noexcept {
    if (_callExitFuncOnDestruct) {
      _exitFunc();
    }
  }

  void Release() noexcept { _callExitFuncOnDestruct = false; }

private:
  bool _callExitFuncOnDestruct;
  ExitFunction _exitFunc;
};

template <typename ExistFunction>
ScopeGuard(ExistFunction)->ScopeGuard<ExistFunction>;
} // namespace CesiumUtility
