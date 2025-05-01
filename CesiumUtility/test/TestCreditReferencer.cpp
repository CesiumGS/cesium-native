#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>

#include <doctest/doctest.h>

#include <sstream>

using namespace CesiumUtility;

TEST_CASE("CreditReferencer") {
  auto pCreditSystem = std::make_shared<CreditSystem>();
  Credit credit1 = pCreditSystem->createCredit("1");
  Credit credit2 = pCreditSystem->createCredit("2");
  Credit credit3 = pCreditSystem->createCredit("3");

  SUBCASE("does nothing if not supplied a CreditSystem") {
    CreditReferencer referencer;
    CHECK(referencer.getCreditSystem() == nullptr);

    referencer.addCreditReference(credit1);
    referencer.releaseAllReferences();
  }

  SUBCASE("adds references to the underlying credit system") {
    auto pReferencer = std::make_unique<CreditReferencer>(pCreditSystem);

    pReferencer->addCreditReference(credit1);
    pReferencer->addCreditReference(credit2);

    // A second reference to credit1
    pReferencer->addCreditReference(credit1);

    const CreditsSnapshot& snapshot1 = pCreditSystem->getSnapshot();
    REQUIRE(snapshot1.currentCredits.size() == 2);

    SUBCASE("and removes them in releaseAllReferences") {
      pReferencer->releaseAllReferences();
      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 0);
    }

    SUBCASE("and removes them in the destructor") {
      pReferencer.reset();
      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 0);
    }

    SUBCASE("and duplicates them in the copy constructor") {
      CreditReferencer referencer2(*pReferencer);

      pReferencer.reset();
      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 2);

      referencer2.releaseAllReferences();
      const CreditsSnapshot& snapshot3 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot3.currentCredits.size() == 0);
    }

    SUBCASE("and moves them in the move constructor") {
      CreditReferencer referencer2(std::move(*pReferencer));

      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 2);

      referencer2.releaseAllReferences();
      const CreditsSnapshot& snapshot3 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot3.currentCredits.size() == 0);
    }

    SUBCASE("and clears them on copy assignment") {
      CreditReferencer referencer2(pCreditSystem);
      referencer2.addCreditReference(credit3);

      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 3);

      referencer2 = *pReferencer;
      const CreditsSnapshot& snapshot3 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot3.currentCredits.size() == 2);

      pReferencer.reset();
      const CreditsSnapshot& snapshot4 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot4.currentCredits.size() == 2);

      referencer2 = CreditReferencer();
      const CreditsSnapshot& snapshot5 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot5.currentCredits.size() == 0);
    }

    SUBCASE("and clears them on move assignment") {
      CreditReferencer referencer2(pCreditSystem);
      referencer2.addCreditReference(credit3);

      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 3);

      referencer2 = std::move(*pReferencer);
      const CreditsSnapshot& snapshot3 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot3.currentCredits.size() == 2);

      referencer2 = CreditReferencer();
      const CreditsSnapshot& snapshot4 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot4.currentCredits.size() == 0);
    }

    SUBCASE("and clears them when the credit system is set to nullptr") {
      pReferencer->setCreditSystem(nullptr);
      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 0);
    }

    SUBCASE("and clears them when the credit system changes") {
      pReferencer->setCreditSystem(std::make_shared<CreditSystem>());
      const CreditsSnapshot& snapshot2 = pCreditSystem->getSnapshot();
      REQUIRE(snapshot2.currentCredits.size() == 0);
    }
  }
}
