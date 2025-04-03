#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>

namespace CesiumUtility {

CreditReferencer::CreditReferencer() noexcept : CreditReferencer(nullptr) {}

CreditReferencer::CreditReferencer(
    const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept
    : _pCreditSystem(pCreditSystem) {}

CreditReferencer::~CreditReferencer() noexcept { this->releaseAllReferences(); }

const std::shared_ptr<CreditSystem>&
CreditReferencer::getCreditSystem() const noexcept {
  return this->_pCreditSystem;
}

void CreditReferencer::setCreditSystem(
    const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept {
  if (this->_pCreditSystem != pCreditSystem) {
    this->releaseAllReferences();
    this->_pCreditSystem = pCreditSystem;
  }
}

void CreditReferencer::addCreditReference(Credit credit) noexcept {
  if (!this->_pCreditSystem)
    return;

  if (this->_references.size() <= credit.id) {
    this->_references.resize(credit.id + 1);
  }

  ++this->_references[credit.id];
  this->_pCreditSystem->addCreditReference(credit);
}

void CreditReferencer::releaseAllReferences() noexcept {
  if (!this->_pCreditSystem)
    return;

  this->_pCreditSystem->releaseBulkReferences(this->_references);

  this->_references.clear();
}

} // namespace CesiumUtility
