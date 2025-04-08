#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace CesiumUtility {

Credit CreditSystem::createCredit(const std::string& html, bool showOnScreen) {
  return this->createCredit(std::string(html), showOnScreen);
}

Credit CreditSystem::createCredit(std::string&& html, bool showOnScreen) {
  // if this credit already exists, return a Credit handle to it
  for (size_t id = 0; id < this->_credits.size(); ++id) {
    if (this->_credits[id].html == html) {
      // Override the existing credit's showOnScreen value.
      this->_credits[id].showOnScreen = showOnScreen;
      return Credit(id);
    }
  }

  this->_credits.push_back({std::move(html), showOnScreen, 0, false});

  return Credit(this->_credits.size() - 1);
}

bool CreditSystem::shouldBeShownOnScreen(Credit credit) const noexcept {
  if (credit.id < this->_credits.size()) {
    return this->_credits[credit.id].showOnScreen;
  }
  return false;
}

void CreditSystem::setShowOnScreen(Credit credit, bool showOnScreen) noexcept {
  if (credit.id < this->_credits.size()) {
    this->_credits[credit.id].showOnScreen = showOnScreen;
  }
}

const std::string& CreditSystem::getHtml(Credit credit) const noexcept {
  if (credit.id < this->_credits.size()) {
    return this->_credits[credit.id].html;
  }
  return INVALID_CREDIT_MESSAGE;
}

void CreditSystem::addCreditReference(Credit credit) {
  CreditRecord& record = this->_credits[credit.id];
  ++record.referenceCount;

  // If this is the first reference to this credit, and it was shown last frame,
  // make sure this credit doesn't exist in _creditsToNoLongerShowThisSnapshot.
  if (record.shownLastSnapshot && record.referenceCount == 1) {
    this->_creditsToNoLongerShowThisSnapshot.erase(
        std::remove(
            this->_creditsToNoLongerShowThisSnapshot.begin(),
            this->_creditsToNoLongerShowThisSnapshot.end(),
            credit),
        this->_creditsToNoLongerShowThisSnapshot.end());
  }
}

void CreditSystem::removeCreditReference(Credit credit) {
  CreditRecord& record = this->_credits[credit.id];
  CESIUM_ASSERT(record.referenceCount > 0);
  --record.referenceCount;

  // If this was the last reference to this credit, and it was shown last frame,
  // add this credit to _creditsToNoLongerShowThisSnapshot.
  if (record.shownLastSnapshot && record.referenceCount == 0) {
    this->_creditsToNoLongerShowThisSnapshot.emplace_back(credit);
  }
}

const CreditsSnapshot& CreditSystem::getSnapshot() noexcept {
  std::vector<Credit>& currentCredits = this->_snapshot.currentCredits;
  currentCredits.clear();

  for (size_t i = 0; i < this->_credits.size(); ++i) {
    CreditRecord& record = this->_credits[i];
    if (record.referenceCount > 0) {
      currentCredits.emplace_back(Credit(i));
      record.shownLastSnapshot = true;
    } else {
      record.shownLastSnapshot = false;
    }
  }

  this->_creditsToNoLongerShowThisSnapshot.swap(this->_snapshot.removedCredits);
  this->_creditsToNoLongerShowThisSnapshot.clear();

  // sort credits based on the number of occurrences
  std::sort(
      currentCredits.begin(),
      currentCredits.end(),
      [this](const Credit& a, const Credit& b) {
        int32_t aCounts = this->_credits[a.id].referenceCount;
        int32_t bCounts = this->_credits[b.id].referenceCount;
        if (aCounts == bCounts)
          return a.id < b.id;
        else
          return aCounts > bCounts;
      });

  return this->_snapshot;
}

void CreditSystem::addBulkReferences(
    const std::vector<int32_t>& references) noexcept {
  for (size_t i = 0; i < references.size(); ++i) {
    CreditRecord& record = this->_credits[i];
    int32_t referencesToAdd = references[i];

    record.referenceCount += referencesToAdd;

    // If this is the first reference to this credit, and it was shown last
    // frame, make sure this credit doesn't exist in
    // _creditsToNoLongerShowThisSnapshot.
    if (record.shownLastSnapshot && record.referenceCount == referencesToAdd) {
      this->_creditsToNoLongerShowThisSnapshot.erase(
          std::remove(
              this->_creditsToNoLongerShowThisSnapshot.begin(),
              this->_creditsToNoLongerShowThisSnapshot.end(),
              Credit(i)),
          this->_creditsToNoLongerShowThisSnapshot.end());
    }
  }
}

void CreditSystem::releaseBulkReferences(
    const std::vector<int32_t>& references) noexcept {
  for (size_t i = 0; i < references.size(); ++i) {
    CreditRecord& record = this->_credits[i];
    int32_t referencesToRemove = references[i];

    CESIUM_ASSERT(record.referenceCount >= referencesToRemove);
    record.referenceCount -= referencesToRemove;

    // If this was the last reference to this credit, and it was shown last
    // frame, add this credit to _creditsToNoLongerShowThisSnapshot.
    if (record.shownLastSnapshot && record.referenceCount == 0) {
      this->_creditsToNoLongerShowThisSnapshot.emplace_back(Credit(i));
    }
  }
}

} // namespace CesiumUtility
