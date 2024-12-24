#pragma once

#include <type_traits>
#include <utility>

namespace CesiumUtility {
/**
 * @brief A utility that will automatically call the lambda function when
 * exiting a scope.
 *
 * @tparam ExitFunction The function type to be called when the guard is out of
 * scope.
 */
template <typename ExitFunction> class ScopeGuard {
public:
  /**
   * @brief Constructor.
   *
   * @param exitFunc The function type to be called when the guard is out
   * of scope
   */
  template <
      typename ExitFunctionArg,
      typename std::enable_if_t<
          !std::is_same_v<
              std::remove_reference_t<std::remove_const_t<ExitFunctionArg>>,
              ScopeGuard<ExitFunction>>,
          int> = 0>
  explicit ScopeGuard(ExitFunctionArg&& exitFunc)
      : _callExitFuncOnDestruct{true},
        _exitFunc{std::forward<ExitFunctionArg>(exitFunc)} {}

  ScopeGuard(const ScopeGuard& rhs) = delete;

  /**
   * @brief Move constructor. The rhs will move its lambda to the lhs, and the
   * rhs will not call its lambda upon exiting a scope
   */
  ScopeGuard(ScopeGuard&& rhs) noexcept
      : _callExitFuncOnDestruct{rhs._callExitFuncOnDestruct},
        _exitFunc{std::move(rhs._exitFunc)} {
    rhs.release();
  }

  ScopeGuard& operator=(const ScopeGuard& rhs) = delete;

  /**
   * @brief Move assignment operator. The rhs will move its lambda to the lhs,
   * and the rhs will not call its lambda upon exiting a scope
   */
  ScopeGuard& operator=(ScopeGuard&& rhs) noexcept {
    if (&rhs != this) {
      _callExitFuncOnDestruct = rhs._callExitFuncOnDestruct;
      _exitFunc = std::move(rhs._exitFunc);
      rhs.release();
    }

    return *this;
  }

  /**
   * @brief Destructor. The guard will execute the lambda function when exiting
   * a scope if it's not released
   */
  ~ScopeGuard() noexcept {
    if (_callExitFuncOnDestruct) {
      _exitFunc();
    }
  }

  /**
   * @brief Upon calling ScopeGuard::release(), the guard will not execute the
   * lambda function when exiting a scope
   */
  void release() noexcept { _callExitFuncOnDestruct = false; }

private:
  bool _callExitFuncOnDestruct;
  ExitFunction _exitFunc;
};

/** @brief Template deduction guide for \ref ScopeGuard to help the compiler
 * figure out that the type of `ExitFunction` should be the type of the
 * function passed to the constructor. */
template <typename ExitFunction>
ScopeGuard(ExitFunction) -> ScopeGuard<ExitFunction>;
} // namespace CesiumUtility
