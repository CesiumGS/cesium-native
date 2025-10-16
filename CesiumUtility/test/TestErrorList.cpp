#include <CesiumUtility/ErrorList.h>

#include <doctest/doctest.h>

TEST_CASE("ErrorList") {
  CesiumUtility::ErrorList errorList;

  SUBCASE("initially has no errors or warnings") {
    CHECK(errorList.errors.empty());
    CHECK(errorList.warnings.empty());
  }

  SUBCASE("can emplace error") {
    errorList.emplaceError("An error occurred");
    CHECK(errorList.errors.size() == 1);
    CHECK(errorList.errors[0] == "An error occurred");
    CHECK(errorList.warnings.empty());
  }

  SUBCASE("can emplace warning") {
    errorList.emplaceWarning("A warning occurred");
    CHECK(errorList.warnings.size() == 1);
    CHECK(errorList.warnings[0] == "A warning occurred");
    CHECK(errorList.errors.empty());
  }

  SUBCASE("formats as empty string when there are no errors or warnings") {
    std::string formatted = errorList.format("The prompt:");
    CHECK(formatted == "");
  }

  SUBCASE("formats one warning") {
    errorList.emplaceWarning("First warning");
    std::string formatted = errorList.format("The prompt:");
    CHECK(formatted == "The prompt:\n- [Warning] First warning");

    SUBCASE("formats multiple warnings") {
      errorList.emplaceWarning("Second warning");
      formatted = errorList.format("The prompt:");
      CHECK(
          formatted ==
          "The prompt:\n- [Warning] First warning\n- [Warning] Second warning");
    }
  }

  SUBCASE("formats one error") {
    errorList.emplaceError("First error");
    std::string formatted = errorList.format("The prompt:");
    CHECK(formatted == "The prompt:\n- [Error] First error");

    SUBCASE("formats multiple errors") {
      errorList.emplaceError("Second error");
      formatted = errorList.format("The prompt:");
      CHECK(
          formatted ==
          "The prompt:\n- [Error] First error\n- [Error] Second error");
    }
  }

  SUBCASE("formats one error and one warning") {
    errorList.emplaceError("First error");
    errorList.emplaceWarning("First warning");
    std::string formatted = errorList.format("The prompt:");
    CHECK(
        formatted ==
        "The prompt:\n- [Error] First error\n- [Warning] First warning");

    SUBCASE("formats multiple errors and warnings") {
      errorList.emplaceError("Second error");
      errorList.emplaceWarning("Second warning");
      formatted = errorList.format("The prompt:");
      CHECK(
          formatted ==
          "The prompt:\n- [Error] First error\n- [Error] Second error\n- "
          "[Warning] First warning\n- [Warning] Second warning");
    }
  }
}