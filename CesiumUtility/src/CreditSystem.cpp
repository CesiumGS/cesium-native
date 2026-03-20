#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace CesiumUtility {

CreditSource::CreditSource(CreditSystem& creditSystem) noexcept
    : _pCreditSystem(&creditSystem) {
  this->_pCreditSystem->createCreditSource(*this);
}

CreditSource::CreditSource(
    const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept
    : _pCreditSystem(pCreditSystem.get()) {
  if (this->_pCreditSystem) {
    this->_pCreditSystem->createCreditSource(*this);
  }
}

CreditSource::~CreditSource() noexcept {
  if (this->_pCreditSystem) {
    this->_pCreditSystem->destroyCreditSource(*this);
  }
}

CreditSystem* CreditSource::getCreditSystem() noexcept {
  return this->_pCreditSystem;
}

const CreditSystem* CreditSource::getCreditSystem() const noexcept {
  return this->_pCreditSystem;
}

void CreditSource::notifyCreditSystemDestroyed() noexcept {
  this->_pCreditSystem = nullptr;
}

CreditSystem::~CreditSystem() noexcept {
  this->_defaultCreditSource.notifyCreditSystemDestroyed();
  for (CreditSource* pSource : this->_creditSources) {
    pSource->notifyCreditSystemDestroyed();
  }
}

Credit CreditSystem::createCredit(
    const CreditSource& source,
    const std::string& html,
    bool showOnScreen) {
  return this->createCredit(source, std::string(html), showOnScreen);
}

Credit CreditSystem::createCredit(
    const CreditSource& source,
    std::string&& html,
    bool showOnScreen) {
  // If this credit already exists, return a Credit handle to it
  uint32_t lastCreditWithSameHtml = INVALID_CREDIT_INDEX;
  for (size_t id = 0; id < this->_credits.size(); ++id) {
    CreditRecord& record = this->_credits[id];
    if (record.html == html) {
      if (record.pSource == &source) {
        // Override the existing credit's showOnScreen value.
        record.showOnScreen = showOnScreen;
        return Credit(uint32_t(id), record.generation);
      } else if (record.pSource != nullptr) {
        // Same HTML, but different source.
        lastCreditWithSameHtml = uint32_t(id);
      }
    }
  }

  // Otherwise, create a new credit record by reusing an existing entry.
  size_t creditIndex;
  if (!this->_unusedCreditRecords.empty()) {
    creditIndex = this->_unusedCreditRecords.back();
    this->_unusedCreditRecords.pop_back();
  } else {
    creditIndex = this->_credits.size();
    this->_credits.emplace_back();
  }

  CreditRecord& record = this->_credits[creditIndex];
  record.html = std::move(html);
  record.showOnScreen = showOnScreen;
  record.referenceCount = 0;
  record.shownLastSnapshot = false;
  // Do not modify `generation` here; it is either initialized to zero for a new
  // Credit slot, or incremented when the previous CreditSource that used this
  // slot was destroyed.
  record.pSource = &source;

  if (lastCreditWithSameHtml != INVALID_CREDIT_INDEX) {
    // Even though `lastCreditWithSameHtml` is the credit with the same HTML
    // that has the largest index, it is not necessarily last in the linked
    // list.
    CreditRecord& lastRecord = this->_credits[size_t(lastCreditWithSameHtml)];
    record.previousCreditWithSameHtml = lastCreditWithSameHtml;
    record.nextCreditWithSameHtml = lastRecord.nextCreditWithSameHtml;
    lastRecord.nextCreditWithSameHtml = uint32_t(creditIndex);

    if (record.nextCreditWithSameHtml != INVALID_CREDIT_INDEX) {
      CreditRecord& nextRecord =
          this->_credits[size_t(record.nextCreditWithSameHtml)];
      nextRecord.previousCreditWithSameHtml = uint32_t(creditIndex);
    }
  }

  return Credit(uint32_t(creditIndex), record.generation);
}

Credit CreditSystem::createCredit(std::string&& html, bool showOnScreen) {
  return this->createCredit(
      this->getDefaultCreditSource(),
      std::move(html),
      showOnScreen);
}

Credit CreditSystem::createCredit(const std::string& html, bool showOnScreen) {
  return this->createCredit(this->getDefaultCreditSource(), html, showOnScreen);
}

bool CreditSystem::shouldBeShownOnScreen(Credit credit) const noexcept {
  if (credit._id < this->_credits.size() &&
      credit._generation == this->_credits[credit._id].generation) {
    return this->_credits[credit._id].showOnScreen;
  }
  return false;
}

void CreditSystem::setShowOnScreen(Credit credit, bool showOnScreen) noexcept {
  if (credit._id < this->_credits.size() &&
      credit._generation == this->_credits[credit._id].generation) {
    this->_credits[credit._id].showOnScreen = showOnScreen;
  }
}

const std::string& CreditSystem::getHtml(Credit credit) const noexcept {
  if (credit._id < this->_credits.size() &&
      credit._generation == this->_credits[credit._id].generation) {
    return this->_credits[credit._id].html;
  }
  return INVALID_CREDIT_MESSAGE;
}

const CreditSource*
CreditSystem::getCreditSource(Credit credit) const noexcept {
  if (credit._id < this->_credits.size() &&
      credit._generation == this->_credits[credit._id].generation) {
    return this->_credits[credit._id].pSource;
  }
  return nullptr;
}

bool CreditSystem::addCreditReference(Credit credit) {
  CreditRecord& record = this->_credits[credit._id];

  // If the Credit is from a previous generation (a deleted credit source),
  // ignore it.
  if (credit._generation != record.generation) {
    return false;
  }

  ++record.referenceCount;

  return true;
}

bool CreditSystem::removeCreditReference(Credit credit) {
  CreditRecord& record = this->_credits[credit._id];
  // If the Credit is from a previous generation (a deleted credit source),
  // ignore it.
  if (credit._generation != record.generation) {
    return false;
  }

  CESIUM_ASSERT(record.referenceCount > 0);
  --record.referenceCount;

  return true;
}

const CreditsSnapshot&
CreditSystem::getSnapshot(CreditFilteringMode filteringMode) noexcept {
  std::vector<Credit>& currentCredits = this->_snapshot.currentCredits;
  std::vector<Credit>& removedCredits = this->_snapshot.removedCredits;
  currentCredits.clear();

  // Any credits that were shown last snapshot and whose sources have since been
  // destroyed need to be reported in removedCredits.
  removedCredits = this->_shownCreditsDestroyed;
  this->_shownCreditsDestroyed.clear();

  std::vector<int32_t>& effectiveReferenceCounts = this->_referenceCountScratch;
  effectiveReferenceCounts.assign(this->_credits.size(), 0);

  for (size_t i = 0; i < this->_credits.size(); ++i) {
    CreditRecord& record = this->_credits[i];

    // A credit whose source has been destroyed will always have a reference
    // count of zero.
    if (record.referenceCount > 0) {
      CESIUM_ASSERT(record.pSource != nullptr);

      // This credit is active, but it may be filtered out in favor of another
      // credit with the same HTML.
      uint32_t filteredInFavorOf =
          this->filterCreditForSnapshot(filteringMode, record);
      if (filteredInFavorOf != INVALID_CREDIT_INDEX) {
        // Credit filtered out in favor of another credit.
        // That other credit inherits this credit's reference count.
        effectiveReferenceCounts[filteredInFavorOf] += record.referenceCount;
        if (record.shownLastSnapshot) {
          removedCredits.emplace_back(Credit(uint32_t(i), record.generation));
          record.shownLastSnapshot = false;
        }
      } else {
        // This credit is not filtered.
        currentCredits.emplace_back(Credit(uint32_t(i), record.generation));
        effectiveReferenceCounts[i] += record.referenceCount;
        record.shownLastSnapshot = true;
      }
    } else if (record.shownLastSnapshot) {
      removedCredits.emplace_back(Credit(uint32_t(i), record.generation));
      record.shownLastSnapshot = false;
    }
  }

  // sort credits based on the number of occurrences
  std::sort(
      currentCredits.begin(),
      currentCredits.end(),
      [&](const Credit& a, const Credit& b) {
        int32_t aCounts = effectiveReferenceCounts[a._id];
        int32_t bCounts = effectiveReferenceCounts[b._id];
        if (aCounts == bCounts)
          return a._id < b._id;
        else
          return aCounts > bCounts;
      });

  return this->_snapshot;
}

const CreditSource& CreditSystem::getDefaultCreditSource() const noexcept {
  return this->_defaultCreditSource;
}

void CreditSystem::addBulkReferences(
    const std::vector<int32_t>& references,
    const std::vector<uint32_t>& generations) noexcept {
  for (size_t i = 0; i < references.size(); ++i) {
    CreditRecord& record = this->_credits[i];
    if (record.generation != generations[i]) {
      // The generations do not match, so these references are to a credit
      // created by a CreditSource that has since been destroyed. Ignore them.
      continue;
    }

    int32_t referencesToAdd = references[i];

    record.referenceCount += referencesToAdd;
  }
}

void CreditSystem::releaseBulkReferences(
    const std::vector<int32_t>& references,
    const std::vector<uint32_t>& generations) noexcept {
  for (size_t i = 0; i < references.size(); ++i) {
    CreditRecord& record = this->_credits[i];
    if (record.generation != generations[i]) {
      // The generations do not match, so these references are to a credit
      // created by a CreditSource that has since been destroyed. Ignore them.
      continue;
    }

    int32_t referencesToRemove = references[i];

    CESIUM_ASSERT(record.referenceCount >= referencesToRemove);
    record.referenceCount -= referencesToRemove;
  }
}

void CreditSystem::createCreditSource(CreditSource& creditSource) noexcept {
  this->_creditSources.emplace_back(&creditSource);
}

void CreditSystem::destroyCreditSource(CreditSource& creditSource) noexcept {
  for (CreditRecord& record : this->_credits) {
    if (record.pSource == &creditSource) {
      if (record.shownLastSnapshot) {
        this->_shownCreditsDestroyed.emplace_back(Credit(
            uint32_t(&record - this->_credits.data()),
            record.generation));
      }

      record.pSource = nullptr;
      ++record.generation;
      this->_unusedCreditRecords.emplace_back(&record - this->_credits.data());

      // Delete this record from the linked list of credits with the same HTML.
      if (record.nextCreditWithSameHtml != INVALID_CREDIT_INDEX) {
        CreditRecord& nextRecord =
            this->_credits[record.nextCreditWithSameHtml];
        nextRecord.previousCreditWithSameHtml =
            record.previousCreditWithSameHtml;
      }

      if (record.previousCreditWithSameHtml != INVALID_CREDIT_INDEX) {
        CreditRecord& previousRecord =
            this->_credits[record.previousCreditWithSameHtml];
        previousRecord.nextCreditWithSameHtml = record.nextCreditWithSameHtml;
      }

      record.referenceCount = 0;
      record.shownLastSnapshot = false;
      record.nextCreditWithSameHtml = INVALID_CREDIT_INDEX;
      record.previousCreditWithSameHtml = INVALID_CREDIT_INDEX;
    }
  }

  this->_creditSources.erase(
      std::remove(
          this->_creditSources.begin(),
          this->_creditSources.end(),
          &creditSource),
      this->_creditSources.end());
}

uint32_t CreditSystem::filterCreditForSnapshot(
    CreditFilteringMode filteringMode,
    const CreditRecord& record) noexcept {
  if (filteringMode == CreditFilteringMode::None) {
    return CreditSystem::INVALID_CREDIT_INDEX;
  } else if (filteringMode == CreditFilteringMode::UniqueHtmlAndShowOnScreen) {
    uint32_t currentCreditIndex = record.previousCreditWithSameHtml;

    while (currentCreditIndex != INVALID_CREDIT_INDEX) {
      const CreditRecord& otherRecord =
          this->_credits[size_t(currentCreditIndex)];

      // If the other credit is referenced and has the same showOnScreen value,
      // filter this one out in favor of it.
      if (otherRecord.referenceCount > 0 &&
          otherRecord.showOnScreen == record.showOnScreen) {
        return currentCreditIndex;
      }

      currentCreditIndex = otherRecord.previousCreditWithSameHtml;
    }

    return CreditSystem::INVALID_CREDIT_INDEX;
  } else {
    CESIUM_ASSERT(filteringMode == CreditFilteringMode::UniqueHtml);

    // In this filtering mode, we need to find the first referenced credit in
    // the linked list with showOnScreen=true. Or if they're all
    // showOnScreen=false, return the first one. Unlike
    // `UniqueHtmlAndShowOnScreen`, the Credit we want may occur after the
    // current one.

    // Walk backwards to find the first referenced credit in the linked list.
    uint32_t previousCreditIndex = uint32_t(&record - this->_credits.data());
    uint32_t firstReferencedCreditIndex = INVALID_CREDIT_INDEX;
    do {
      const CreditRecord& otherRecord =
          this->_credits[size_t(previousCreditIndex)];
      if (otherRecord.referenceCount > 0) {
        firstReferencedCreditIndex = previousCreditIndex;
      }
      previousCreditIndex = otherRecord.previousCreditWithSameHtml;
    } while (previousCreditIndex != INVALID_CREDIT_INDEX);

    // Walk forward from the first referenced credit to find one with
    // showOnScreen=true (if any).
    uint32_t currentCreditIndex = firstReferencedCreditIndex;
    while (currentCreditIndex != INVALID_CREDIT_INDEX) {
      const CreditRecord& otherRecord =
          this->_credits[size_t(currentCreditIndex)];

      if (otherRecord.showOnScreen && otherRecord.referenceCount > 0) {
        // Found the first referenced credit with showOnScreen=true. Filter this
        // credit out in favor of it. Unless the currentCreditIndex points to
        // the same credit we started with!
        return currentCreditIndex == uint32_t(&record - this->_credits.data())
                   ? CreditSystem::INVALID_CREDIT_INDEX
                   : currentCreditIndex;
      }

      currentCreditIndex = otherRecord.nextCreditWithSameHtml;
    }

    // Reached the end of the linked list without finding any credit with
    // showOnScreen=true. So return the first referenced credit in the list.
    // Unless the firstReferencedCreditIndex points to the same credit we
    // started with!
    return firstReferencedCreditIndex ==
                   uint32_t(&record - this->_credits.data())
               ? CreditSystem::INVALID_CREDIT_INDEX
               : firstReferencedCreditIndex;
  }
}

} // namespace CesiumUtility
