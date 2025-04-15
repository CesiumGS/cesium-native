#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>

#include <memory>
#include <utility>

namespace CesiumUtility {

CreditReferencer::CreditReferencer() noexcept : CreditReferencer(nullptr) {}

CreditReferencer::CreditReferencer(
    const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept
    : _pCreditSystem(pCreditSystem) {}

CreditReferencer::CreditReferencer(const CreditReferencer& rhs) noexcept
    : _pCreditSystem(rhs._pCreditSystem), _references(rhs._references) {
  if (this->_pCreditSystem) {
    this->_pCreditSystem->addBulkReferences(this->_references);
  }
}

CreditReferencer&
CreditReferencer::operator=(const CreditReferencer& rhs) noexcept {
  if (this != &rhs) {
    if (this->_pCreditSystem) {
      this->_pCreditSystem->addBulkReferences(rhs._references);
    }

    this->releaseAllReferences();
    this->_pCreditSystem = rhs._pCreditSystem;
    this->_references = rhs._references;
  }

  return *this;
}

CreditReferencer::CreditReferencer(CreditReferencer&& rhs) noexcept = default;

CreditReferencer::~CreditReferencer() noexcept { this->releaseAllReferences(); }

CreditReferencer& CreditReferencer::operator=(CreditReferencer&& rhs) noexcept {
  if (this != &rhs) {
    this->releaseAllReferences();
    this->_pCreditSystem = std::move(rhs._pCreditSystem);
    this->_references = std::move(rhs._references);
  }

  return *this;
}

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
