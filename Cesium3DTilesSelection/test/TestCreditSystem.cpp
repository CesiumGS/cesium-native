#include "Cesium3DTilesSelection/CreditSystem.h"

#include <catch2/catch.hpp>

using namespace Cesium3DTilesSelection;

TEST_CASE("Test sorting credits by frequency") {
  CreditSystem creditSystem;

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";
  std::string html2 = "<html>Credit2</html>";

  Credit credit0 = creditSystem.createCredit(html0);
  Credit credit1 = creditSystem.createCredit(html1);
  Credit credit2 = creditSystem.createCredit(html2);

  creditSystem.addCreditToFrame(credit0);
  for (int i = 0; i < 2; i++) {
    creditSystem.addCreditToFrame(credit1);
  }
  for (int i = 0; i < 3; i++) {
    creditSystem.addCreditToFrame(credit2);
  }

  std::vector<Credit> expectedShow{credit2, credit1, credit0};
  REQUIRE(creditSystem.getCreditsToShowThisFrame() == expectedShow);
}

TEST_CASE("Test basic credit handling") {
  CreditSystem creditSystem;

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";
  std::string html2 = "<html>Credit2</html>";

  Credit credit0 = creditSystem.createCredit(html0);
  Credit credit1 = creditSystem.createCredit(html1);
  Credit credit2 = creditSystem.createCredit(html2);

  creditSystem.addCreditToFrame(credit0);
  for (int i = 0; i < 2; i++) {
    creditSystem.addCreditToFrame(credit1);
  }
  for (int i = 0; i < 3; i++) {
    creditSystem.addCreditToFrame(credit2);
  }

  std::vector<Credit> expectedShow{ credit2, credit1, credit0 };
  REQUIRE(creditSystem.getCreditsToShowThisFrame() == expectedShow);
}

TEST_CASE("Test wrong credit handling") {

  CreditSystem creditSystemA;
  CreditSystem creditSystemB;

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";

  Credit creditA0 = creditSystemA.createCredit(html0);
  Credit creditA1 = creditSystemA.createCredit(html1);

  /*Credit creditB0 = */ creditSystemB.createCredit(html0);

  // NOTE: This is using a Credit from a different credit
  // system, which coincidentally has a valid ID here.
  // This is  not (and can hardly be) checked right now,
  // so this returns a valid HTML string:
  REQUIRE(creditSystemB.getHtml(creditA0) == html0);

  REQUIRE(creditSystemB.getHtml(creditA1) != html1);
}
