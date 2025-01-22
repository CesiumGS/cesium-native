#include <CesiumUtility/IntrusivePointer.h>

#include <doctest/doctest.h>

#include <utility>

using namespace CesiumUtility;

namespace {
class Base {
public:
  int _references = 0;
  void addReference() noexcept { ++_references; };
  void releaseReference() noexcept { --_references; };
};

class Derived : public Base {};

void createCopy(IntrusivePointer<Derived>& pDerived) {
  IntrusivePointer<Base> pBase = pDerived;
  CHECK(pBase->_references == 2);
  CHECK(pBase == pDerived);
  return;
}
} // namespace
TEST_CASE("IntrusivePointer") {
  IntrusivePointer<Derived> pDerived = IntrusivePointer<Derived>(new Derived());
  createCopy(pDerived);
  CHECK(pDerived->_references == 1);
  IntrusivePointer<Base> pBase = std::move(pDerived);
  CHECK(pBase->_references == 1);
  CHECK(pBase != pDerived);
}
