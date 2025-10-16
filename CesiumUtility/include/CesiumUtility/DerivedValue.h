#pragma once

#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Library.h>

#include <optional>
#include <type_traits>

namespace CesiumUtility {

/**
 * @brief A function object that caches the last result of a derivation function
 * based on its input. This is sometimes called "memoization", but this simple
 * implementation only remembers the result of a single past invocation.
 *
 * Please note that this class is not thread-safe.
 *
 * @tparam TInput The type of parameter passed as input to the derivation
 * function. It must be possible (and, preferably, efficient) to compare
 * instances of this type using `operator!=`.
 * @tparam TDerivation The type of the derivation function. This must be a
 * callable type that takes a single parameter of type `TInput` and returns a
 * non-void value.
 */
template <typename TInput, typename TDerivation>
class CESIUMUTILITY_API DerivedValue {
public:
  /**
   * @brief The type returned by the derivation function.
   */
  using TOutput = std::invoke_result_t<TDerivation, TInput>;

  /**
   * @brief Constructs a new instance that will use the given derivation
   * function.
   */
  DerivedValue(const TDerivation& derivation)
      : _derivation(derivation), _lastInput(), _lastOutput() {}

  /**
   * @brief Constructs a new instance that will use the given derivation
   * function.
   */
  DerivedValue(TDerivation&& derivation)
      : _derivation(std::move(derivation)), _lastInput(), _lastOutput() {}

  /**
   * @brief Gets or computes the derived value.
   *
   * If this is the first time this method was invoked, or if the given `input`
   * is different from the last time this method was invoked (as indicated by
   * `operator!=`), the derivation function will be called to compute the
   * derived value. The derived value will be cached and returned.
   *
   * Otherwise, the previously-computed value will be returned.
   */
  template <typename T> TOutput operator()(T&& input) {
    if (input != this->_lastInput) {
      this->_lastInput.emplace(std::forward<T>(input));
      this->_lastOutput.emplace(this->_derivation(*this->_lastInput));
    }

    CESIUM_ASSERT(this->_lastOutput);
    return *this->_lastOutput;
  }

private:
  TDerivation _derivation;
  std::optional<TInput> _lastInput;
  std::optional<TOutput> _lastOutput;
};

/**
 * @brief Helper factory to construct a \ref DerivedValue while specifying only
 * the input type explicitly and letting the callable type (`TDerivation`) be
 * deduced.
 */
template <typename TInput, typename TDerivation>
auto makeDerivedValue(TDerivation&& derivation)
    -> DerivedValue<TInput, std::decay_t<TDerivation>> {
  static_assert(
      std::is_invocable_r_v<
          std::invoke_result_t<std::decay_t<TDerivation>, TInput>,
          std::decay_t<TDerivation>,
          TInput>,
      "Callable provided to makeDerivedValue is not invocable with the "
      "specified TInput.");
  return DerivedValue<TInput, std::decay_t<TDerivation>>(
      std::forward<TDerivation>(derivation));
}

} // namespace CesiumUtility
