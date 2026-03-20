#include <CesiumUtility/DerivedValue.h>

#include <doctest/doctest.h>

using namespace CesiumUtility;

TEST_CASE("DerivedValue") {
  int derivationCallCount = 0;
  auto derivation = [&derivationCallCount](int x) {
    ++derivationCallCount;
    return x * x;
  };

  DerivedValue derivedValue = makeDerivedValue<int>(derivation);

  // First call with input 3, should call derivation
  int result1 = derivedValue(3);
  CHECK(result1 == 9);
  CHECK(derivationCallCount == 1);

  // Second call with same input, should not call derivation
  int result2 = derivedValue(3);
  CHECK(result2 == 9);
  CHECK(derivationCallCount == 1);

  // Call with different input, should call derivation again
  int result3 = derivedValue(4);
  CHECK(result3 == 16);
  CHECK(derivationCallCount == 2);

  // Call again with previous input, should call derivation again
  int result4 = derivedValue(3);
  CHECK(result4 == 9);
  CHECK(derivationCallCount == 3);
}
