#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/JsonHandler.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace CesiumJsonReader;

namespace {

class WarningRecordingParent : public JsonHandler {
public:
  std::vector<std::string> warnings;

  virtual void reportWarning(
      const std::string& warning,
      std::vector<std::string>&& context) override {
    (void)context;
    this->warnings.emplace_back(warning);
  }
};

// Feeds an integral double to an IntegerJsonHandler<T> and returns true if it
// was accepted (no warning reported).
template <typename T> bool readIntegralDouble(double d, T& value) {
  WarningRecordingParent parent;
  IntegerJsonHandler<T> handler;
  handler.reset(&parent, &value);
  handler.readDouble(d);
  return parent.warnings.empty();
}

} // namespace

TEST_CASE(
    "IntegerJsonHandler rejects integral doubles outside the range of "
    "the target type") {
  SUBCASE("int64_t upper boundary") {
    // 2^63 is the first invalid value for int64_t, and it is exactly what
    // static_cast<double>(int64 max) rounds up to, so it must be rejected
    // rather than slipping past a rounded bound.
    int64_t value = 42;
    CHECK(!readIntegralDouble(9223372036854775808.0, value));
    CHECK(value == 42);

    // The largest double below 2^63 is representable as int64 and must parse.
    CHECK(readIntegralDouble(9223372036854774784.0, value));
    CHECK(value == 9223372036854774784LL);
  }

  SUBCASE("int64_t lower boundary") {
    // -2^63 is exactly representable as both double and int64.
    int64_t value = 42;
    CHECK(readIntegralDouble(-9223372036854775808.0, value));
    CHECK(value == std::numeric_limits<int64_t>::lowest());

    // The next double below -2^63 is out of range and must be rejected.
    value = 42;
    CHECK(!readIntegralDouble(-9223372036854777856.0, value));
    CHECK(value == 42);
  }

  SUBCASE("uint64_t upper boundary") {
    // 2^64 is the first invalid value for uint64_t, and it is exactly what
    // static_cast<double>(uint64 max) rounds up to.
    uint64_t value = 42;
    CHECK(!readIntegralDouble(18446744073709551616.0, value));
    CHECK(value == 42);

    // The largest double below 2^64 is representable as uint64 and must
    // parse.
    CHECK(readIntegralDouble(18446744073709549568.0, value));
    CHECK(value == 18446744073709549568ULL);
  }

  SUBCASE("uint64_t lower boundary") {
    // Negative values are out of range for an unsigned type.
    uint64_t value = 42;
    CHECK(!readIntegralDouble(-1.0, value));
    CHECK(value == 42);
  }
}
