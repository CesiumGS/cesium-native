#include <CesiumUtility/ScopeGuard.h>

#include <doctest/doctest.h>

#include <utility>

namespace {
struct ExitFunctor {
  void operator()() { ++(*check); }

  int* check{nullptr};
};
} // namespace

TEST_CASE("Test constructor") {
  bool check = false;
  {
    CesiumUtility::ScopeGuard guard{[&check]() mutable { check = true; }};
  }

  CHECK(check == true);
}

TEST_CASE("Test move constructor") {
  int check = 0;
  {
    CesiumUtility::ScopeGuard rhs{[&check]() mutable { ++check; }};
    CesiumUtility::ScopeGuard lhs{std::move(rhs)};
  }

  CHECK(check == 1);
}

TEST_CASE("Test move operator") {
  int check = 0;
  {
    CesiumUtility::ScopeGuard rhs{ExitFunctor{&check}};
    CesiumUtility::ScopeGuard lhs{ExitFunctor{&check}};
    lhs = std::move(rhs);
  }

  CHECK(check == 1);
}

TEST_CASE("Test release()") {
  int check = 0;
  {
    CesiumUtility::ScopeGuard guard{ExitFunctor{&check}};
    guard.release();
  }

  CHECK(check == 0);
}
