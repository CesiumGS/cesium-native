#include "CesiumUtility/IntrusivePointer.h"

#include <catch2/catch_test_macros.hpp>

#include <utility>

using namespace CesiumUtility;

namespace {
class Base {
public:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  int _references = 0;
  void addReference() noexcept { ++_references; };
  void releaseReference() noexcept { --_references; };
};

class Derived : public Base {};

void createCopy(IntrusivePointer<Derived>& pDerived) {
  IntrusivePointer<Base> pBase = pDerived;
  CHECK(pBase->_references == 2);
  CHECK(pBase == pDerived);
}
} // namespace
TEST_CASE("IntrusivePointer") {
  IntrusivePointer<Derived> pDerived = IntrusivePointer<Derived>(new Derived());
  createCopy(pDerived);
  CHECK(pDerived->_references == 1);
  IntrusivePointer<Base> pBase = std::move(pDerived);
  CHECK(pBase->_references == 1);
  // NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
  CHECK(pBase != pDerived);
}
