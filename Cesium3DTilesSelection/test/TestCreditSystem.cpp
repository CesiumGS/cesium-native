#include <CesiumUtility/CreditSystem.h>

#include <doctest/doctest.h>

#include <optional>
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

TEST_CASE("Test CreditSystem with CreditSources") {
  CreditSystem creditSystem;
  CreditSource sourceA(creditSystem);
  CreditSource sourceB(creditSystem);

  std::string html0 = "<html>Credit0</html>";
  std::string html1 = "<html>Credit1</html>";
  std::string html2 = "<html>Credit2</html>";

  SUBCASE("can create credits from multiple sources") {
    Credit credit0 = creditSystem.createCredit(sourceA, html0);
    Credit credit1 = creditSystem.createCredit(sourceB, html1);

    CHECK(creditSystem.getCreditSource(credit0) == &sourceA);
    CHECK(creditSystem.getHtml(credit0) == html0);
    CHECK(creditSystem.getCreditSource(credit1) == &sourceB);
    CHECK(creditSystem.getHtml(credit1) == html1);
  }

  SUBCASE("credits become invalid when their source is destroyed") {
    std::optional<Credit> credit;

    {
      CreditSource tempSourceA(creditSystem);
      credit = creditSystem.createCredit(tempSourceA, html0);
      CHECK(creditSystem.getCreditSource(*credit) == &tempSourceA);
    }

    // tempSourceA is destroyed here.
    // The credit should no longer have a source.
    CHECK(creditSystem.getCreditSource(*credit) == nullptr);
    // Getting HTML from a credit with no source should not crash and it provide
    // a non-empty (error) message.
    CHECK(!creditSystem.getHtml(*credit).empty());

    // Creating a new credit from a different source should still work.
    CreditSource tempSourceB(creditSystem);
    Credit credit1 = creditSystem.createCredit(tempSourceB, html1);
    CHECK(creditSystem.getCreditSource(credit1) == &tempSourceB);
    CHECK(creditSystem.getHtml(credit1) == html1);
  }

  SUBCASE("when the source of a credit last frame is destroyed, that credit is "
          "not reported") {
    std::unique_ptr<CreditSource> pTempSourceA =
        std::make_unique<CreditSource>(creditSystem);
    Credit credit0 = creditSystem.createCredit(*pTempSourceA, html0);

    creditSystem.addCreditReference(credit0);
    const CreditsSnapshot& snapshot0 = creditSystem.getSnapshot();
    CHECK(snapshot0.currentCredits.size() == 1);
    CHECK(snapshot0.currentCredits[0] == credit0);

    // Remove the credit reference, which will add it to the list of "no longer
    // shown" credits.
    creditSystem.removeCreditReference(credit0);

    // Destroy the source.
    // The credit should no longer be reported in the next snapshot.
    pTempSourceA.reset();

    const CreditsSnapshot& snapshot1 = creditSystem.getSnapshot();
    CHECK(snapshot1.currentCredits.empty());
    CHECK(snapshot1.removedCredits.empty());
  }

  SUBCASE("CreditSystem may be destroyed before CreditSource") {
    std::unique_ptr<CreditSystem> pCreditSystem =
        std::make_unique<CreditSystem>();
    std::unique_ptr<CreditSource> pSource =
        std::make_unique<CreditSource>(*pCreditSystem);

    // Destroy the system, then the source. This should not crash.
    pCreditSystem.reset();
    pSource.reset();
  }

  SUBCASE("two strings from different sources produce different credits") {
    Credit credit0 = creditSystem.createCredit(sourceA, html0);
    Credit credit1 = creditSystem.createCredit(sourceB, html0);
    CHECK(credit0 != credit1);
  }

  SUBCASE("two strings from the same source produce the same credit") {
    Credit credit0 = creditSystem.createCredit(sourceA, html0);
    Credit credit1 = creditSystem.createCredit(sourceA, html0);
    CHECK(credit0 == credit1);
  }

  SUBCASE("destroying a source resets the reference counts of its credits") {
    std::unique_ptr<CreditSource> pSourceTemp =
        std::make_unique<CreditSource>(creditSystem);
    Credit credit0 = creditSystem.createCredit(*pSourceTemp, html0);
    creditSystem.addCreditReference(credit0);

    pSourceTemp.reset();

    creditSystem.createCredit(sourceA, html0);

    const CreditsSnapshot& snapshot = creditSystem.getSnapshot();
    CHECK(snapshot.currentCredits.empty());
    CHECK(snapshot.removedCredits.empty());
  }

  SUBCASE(
      "releasing a reference to a credit from a destroyed source is a no-op") {
    std::unique_ptr<CreditSource> pSourceTemp =
        std::make_unique<CreditSource>(creditSystem);
    Credit credit0 = creditSystem.createCredit(*pSourceTemp, html0);
    creditSystem.addCreditReference(credit0);

    pSourceTemp.reset();

    // Create another credit in a new source, which will reuse the same credit
    // record.
    Credit credit1 = creditSystem.createCredit(sourceA, html0);
    creditSystem.addCreditReference(credit1);

    // This should be a no-op and not crash.
    creditSystem.removeCreditReference(credit0);

    const CreditsSnapshot& snapshot = creditSystem.getSnapshot();
    REQUIRE(snapshot.currentCredits.size() == 1);
    CHECK(snapshot.currentCredits[0] == credit1);
    CHECK(snapshot.removedCredits.empty());
  }

  SUBCASE("getSnapshot") {
    SUBCASE("None filtering mode") {
      SUBCASE("includes all Credits from all sources") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0);
        Credit credit1 = creditSystem.createCredit(sourceB, html0);
        Credit credit2 = creditSystem.createCredit(sourceC, html0);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot =
            creditSystem.getSnapshot(CreditFilteringMode::None);

        REQUIRE(snapshot.currentCredits.size() == 3);
        CHECK(snapshot.currentCredits[0] == credit0);
        CHECK(snapshot.currentCredits[1] == credit1);
        CHECK(snapshot.currentCredits[2] == credit2);
        CHECK(snapshot.removedCredits.empty());
      }
    }

    SUBCASE("UniqueHtmlAndShowOnScreen filtering mode") {
      SUBCASE(
          "filters out credits with identical HTML and showOnScreen values") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0, true);
        Credit credit1 = creditSystem.createCredit(sourceB, html0, true);
        Credit credit2 = creditSystem.createCredit(sourceC, html0, false);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot = creditSystem.getSnapshot(
            CreditFilteringMode::UniqueHtmlAndShowOnScreen);

        REQUIRE(snapshot.currentCredits.size() == 2);
        CHECK(snapshot.currentCredits[0] == credit0);
        CHECK(snapshot.currentCredits[1] == credit2);
        CHECK(snapshot.removedCredits.empty());
      }

      SUBCASE("does not filter in favor of unreferenced credits") {
        CreditSource sourceC(creditSystem);
        [[maybe_unused]] Credit credit0 =
            creditSystem.createCredit(sourceA, html0, true);
        Credit credit1 = creditSystem.createCredit(sourceB, html0, true);
        Credit credit2 = creditSystem.createCredit(sourceC, html0, false);

        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);
        // Note: credit0 is not referenced.

        const CreditsSnapshot& snapshot = creditSystem.getSnapshot(
            CreditFilteringMode::UniqueHtmlAndShowOnScreen);

        REQUIRE(snapshot.currentCredits.size() == 2);
        CHECK(snapshot.currentCredits[0] == credit1);
        CHECK(snapshot.currentCredits[1] == credit2);
        CHECK(snapshot.removedCredits.empty());
      }

      SUBCASE("reference count is the sum of collapsed credits") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0, false);
        Credit credit1 = creditSystem.createCredit(sourceB, html0, true);
        Credit credit2 = creditSystem.createCredit(sourceC, html0, true);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot = creditSystem.getSnapshot(
            CreditFilteringMode::UniqueHtmlAndShowOnScreen);

        // credit0 has a reference count of 2. credit1 and credit2 are collapsed
        // into one credit with a reference count of 3 and represented by
        // credit1. So credit1 should be shown before credit0.
        REQUIRE(snapshot.currentCredits.size() == 2);
        CHECK(snapshot.currentCredits[0] == credit1);
        CHECK(snapshot.currentCredits[1] == credit0);
        CHECK(snapshot.removedCredits.empty());
      }
    }

    SUBCASE("UniqueHtml filtering mode") {
      SUBCASE(
          "filters out credits with identical HTML from different sources") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0);
        Credit credit1 = creditSystem.createCredit(sourceB, html0);
        Credit credit2 = creditSystem.createCredit(sourceC, html0);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot =
            creditSystem.getSnapshot(CreditFilteringMode::UniqueHtml);

        REQUIRE(snapshot.currentCredits.size() == 1);
        CHECK(snapshot.currentCredits[0] == credit0);
        CHECK(snapshot.removedCredits.empty());
      }

      SUBCASE("includes the Credit with showOnScreen=true if one exists.") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0, false);
        Credit credit1 = creditSystem.createCredit(sourceB, html0, true);
        Credit credit2 = creditSystem.createCredit(sourceC, html0, false);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot =
            creditSystem.getSnapshot(CreditFilteringMode::UniqueHtml);

        REQUIRE(snapshot.currentCredits.size() == 1);
        CHECK(snapshot.currentCredits[0] == credit1);
        CHECK(snapshot.removedCredits.empty());
      }

      SUBCASE("includes the first of multiple Credits with showOnScreen=true") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0, true);
        Credit credit1 = creditSystem.createCredit(sourceB, html0, true);
        Credit credit2 = creditSystem.createCredit(sourceC, html0, false);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot =
            creditSystem.getSnapshot(CreditFilteringMode::UniqueHtml);

        REQUIRE(snapshot.currentCredits.size() == 1);
        CHECK(snapshot.currentCredits[0] == credit0);
        CHECK(snapshot.removedCredits.empty());
      }

      SUBCASE("does not filter in favor of unreferenced credits") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0, false);
        Credit credit1 = creditSystem.createCredit(sourceB, html0, false);
        [[maybe_unused]] Credit credit2 =
            creditSystem.createCredit(sourceC, html0, true);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        // Note: credit2 is not referenced.

        const CreditsSnapshot& snapshot =
            creditSystem.getSnapshot(CreditFilteringMode::UniqueHtml);

        REQUIRE(snapshot.currentCredits.size() == 1);
        CHECK(snapshot.currentCredits[0] == credit0);
        CHECK(snapshot.removedCredits.empty());
      }

      SUBCASE("reference count is the sum of collapsed credits") {
        CreditSource sourceC(creditSystem);
        Credit credit0 = creditSystem.createCredit(sourceA, html0);
        Credit credit1 = creditSystem.createCredit(sourceB, html1);
        Credit credit2 = creditSystem.createCredit(sourceC, html1);

        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit0);
        creditSystem.addCreditReference(credit1);
        creditSystem.addCreditReference(credit2);
        creditSystem.addCreditReference(credit2);

        const CreditsSnapshot& snapshot =
            creditSystem.getSnapshot(CreditFilteringMode::UniqueHtml);

        // credit0 has a reference count of 2. credit1 and credit2 are collapsed
        // into one credit with a reference count of 3 and represented by
        // credit1. So credit1 should be shown before credit0.
        REQUIRE(snapshot.currentCredits.size() == 2);
        CHECK(snapshot.currentCredits[0] == credit1);
        CHECK(snapshot.currentCredits[1] == credit0);
        CHECK(snapshot.removedCredits.empty());
      }
    }
  }

  // Refeference count for sorting is the sum of collapsed credit reference
  // counts.
}