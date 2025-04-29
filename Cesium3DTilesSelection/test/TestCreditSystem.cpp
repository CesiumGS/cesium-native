#include <CesiumUtility/CreditSystem.h>

#include <doctest/doctest.h>

#include <string>
#include <vector>

using namespace CesiumUtility;

TEST_CASE("Test basic credit handling") {

  CreditSystem creditSystem;

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";
  std::string html2 = "<html>Credit2</html>";

  Credit credit0 = creditSystem.createCredit(html0);
  Credit credit1 = creditSystem.createCredit(html1);
  Credit credit2 = creditSystem.createCredit(html2);

  REQUIRE(creditSystem.getHtml(credit1) == html1);

  // Frame 0: Add 0 and 1
  {
    creditSystem.addCreditReference(credit0);
    creditSystem.addCreditReference(credit1);
    const CreditsSnapshot& snapshot0 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow0{credit0, credit1};
    REQUIRE(snapshot0.currentCredits == expectedShow0);

    std::vector<Credit> expectedHide0{};
    REQUIRE(snapshot0.removedCredits == expectedHide0);
  }

  // Start frame 1: Add 2, remove 0
  {
    creditSystem.addCreditReference(credit2);
    creditSystem.removeCreditReference(credit0);
    const CreditsSnapshot& snapshot1 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow1{credit1, credit2};
    REQUIRE(snapshot1.currentCredits == expectedShow1);

    std::vector<Credit> expectedHide1{credit0};
    REQUIRE(snapshot1.removedCredits == expectedHide1);
  }

  // Start frame 2: Add nothing, remove 1 and 2
  {
    creditSystem.removeCreditReference(credit1);
    creditSystem.removeCreditReference(credit2);
    const CreditsSnapshot& snapshot2 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow2{};
    REQUIRE(snapshot2.currentCredits == expectedShow2);

    std::vector<Credit> expectedHide2{credit1, credit2};
    REQUIRE(snapshot2.removedCredits == expectedHide2);
  }

  // Start frame 3: Add nothing, remove nothing
  {
    const CreditsSnapshot& snapshot3 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow3{};
    REQUIRE(snapshot3.currentCredits == expectedShow3);

    std::vector<Credit> expectedHide3{};
    REQUIRE(snapshot3.removedCredits == expectedHide3);
  }

  // Start frame 4: Add 2, remove nothing
  {
    creditSystem.addCreditReference(credit2);
    const CreditsSnapshot& snapshot4 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow4{credit2};
    REQUIRE(snapshot4.currentCredits == expectedShow4);

    std::vector<Credit> expectedHide4{};
    REQUIRE(snapshot4.removedCredits == expectedHide4);
  }

  // Start frame 5: Remove and then re-add 2
  {
    creditSystem.removeCreditReference(credit2);
    creditSystem.addCreditReference(credit2);
    const CreditsSnapshot& snapshot5 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow5{credit2};
    REQUIRE(snapshot5.currentCredits == expectedShow5);

    std::vector<Credit> expectedHide5{};
    REQUIRE(snapshot5.removedCredits == expectedHide5);
  }

  // Start frame 6: Add and then remove 1
  {
    creditSystem.addCreditReference(credit1);
    creditSystem.removeCreditReference(credit1);
    const CreditsSnapshot& snapshot6 = creditSystem.getSnapshot();

    std::vector<Credit> expectedShow6{credit2};
    REQUIRE(snapshot6.currentCredits == expectedShow6);

    std::vector<Credit> expectedHide6{};
    REQUIRE(snapshot6.removedCredits == expectedHide6);
  }
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
  // This is not (and can hardly be) checked right now,
  // so this returns a valid HTML string:
  REQUIRE(creditSystemB.getHtml(creditA0) == html0);

  REQUIRE(creditSystemB.getHtml(creditA1) != html1);
}

TEST_CASE("Test sorting credits by frequency") {

  CreditSystem creditSystem;

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";
  std::string html2 = "<html>Credit2</html>";

  Credit credit0 = creditSystem.createCredit(html0);
  Credit credit1 = creditSystem.createCredit(html1);
  Credit credit2 = creditSystem.createCredit(html2);

  REQUIRE(creditSystem.getHtml(credit1) == html1);

  for (int i = 0; i < 3; i++) {
    creditSystem.addCreditReference(credit0);
  }
  for (int i = 0; i < 2; i++) {
    creditSystem.addCreditReference(credit1);
  }
  creditSystem.addCreditReference(credit2);

  const CreditsSnapshot& snapshot0 = creditSystem.getSnapshot();

  std::vector<Credit> expectedShow0{credit0, credit1, credit2};
  REQUIRE(snapshot0.currentCredits == expectedShow0);

  for (int i = 0; i < 2; i++) {
    creditSystem.addCreditReference(credit2);
  }
  for (int i = 0; i < 2; i++) {
    creditSystem.removeCreditReference(credit0);
  }

  const CreditsSnapshot& snapshot1 = creditSystem.getSnapshot();

  std::vector<Credit> expectedShow1{credit2, credit1, credit0};
  REQUIRE(snapshot1.currentCredits == expectedShow1);
}

TEST_CASE("Test setting showOnScreen on credits") {

  CreditSystem creditSystem;

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";
  std::string html2 = "<html>Credit2</html>";

  Credit credit0 = creditSystem.createCredit(html0, true);
  Credit credit1 = creditSystem.createCredit(html1, false);
  Credit credit2 = creditSystem.createCredit(html2, true);

  REQUIRE(creditSystem.getHtml(credit1) == html1);

  CHECK(creditSystem.shouldBeShownOnScreen(credit0) == true);
  CHECK(creditSystem.shouldBeShownOnScreen(credit1) == false);
  CHECK(creditSystem.shouldBeShownOnScreen(credit2) == true);

  creditSystem.setShowOnScreen(credit0, false);
  creditSystem.setShowOnScreen(credit1, true);
  creditSystem.setShowOnScreen(credit2, true);

  CHECK(creditSystem.shouldBeShownOnScreen(credit0) == false);
  CHECK(creditSystem.shouldBeShownOnScreen(credit1) == true);
  CHECK(creditSystem.shouldBeShownOnScreen(credit2) == true);
}
