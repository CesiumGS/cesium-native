#pragma once

#include <type_traits>

namespace CesiumUtility {

template <typename TInput, typename TDerivation>
class CESIUMUTILITY_API DerivedValue {
public:
  using TOutput = std::invoke_result_t<TDerivation, TInput>;

  DerivedValue(const TDerivation& derivation)
      : _derivation(derivation), _lastInput(), _lastOutput() {}
  DerivedValue(TDerivation&& derivation)
      : _derivation(std::move(derivation)), _lastInput(), _lastOutput() {}

  TOutput operator()(TInput input) {
    if (input != this->_lastInput) {
      this->_lastInput.emplace(std::move(input));
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

} // namespace CesiumUtility
