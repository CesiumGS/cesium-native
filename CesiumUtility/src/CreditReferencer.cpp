#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>

#include <memory>
#include <utility>

namespace CesiumUtility {

CreditReferencer::CreditReferencer() noexcept : CreditReferencer(nullptr) {}

CreditReferencer::CreditReferencer(
    const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept
    : _pCreditSystem(pCreditSystem), _references(), _generations() {}

CreditReferencer::CreditReferencer(const CreditReferencer& rhs) noexcept
    : _pCreditSystem(rhs._pCreditSystem),
      _references(rhs._references),
      _generations(rhs._generations) {
  if (this->_pCreditSystem) {
    this->_pCreditSystem->addBulkReferences(
        this->_references,
        this->_generations);
  }
}

CreditReferencer&
CreditReferencer::operator=(const CreditReferencer& rhs) noexcept {
  if (this != &rhs) {
    if (this->_pCreditSystem) {
      this->_pCreditSystem->addBulkReferences(
          rhs._references,
          rhs._generations);
    }

    this->releaseAllReferences();
    this->_pCreditSystem = rhs._pCreditSystem;
    this->_references = rhs._references;
    this->_generations = rhs._generations;
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
    this->_generations = std::move(rhs._generations);
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

  if (!this->_pCreditSystem->addCreditReference(credit)) {
    // This credit was created by a CreditSource that has been destroyed. Ignore
    // it.
    return;
  }

  if (this->_references.size() <= credit._id) {
    this->_references.resize(credit._id + 1);
    this->_generations.resize(credit._id + 1);
  }

  if (this->_generations[credit._id] != credit._generation) {
    // The existing references (if any) refer to an older generation of this
    // credit (i.e., from a previous CreditSource that has been destroyed). So
    // reset the reference count to zero.
    this->_references[credit._id] = 0;
    this->_generations[credit._id] = credit._generation;
  }

  ++this->_references[credit._id];
}

void CreditReferencer::releaseAllReferences() noexcept {
  if (!this->_pCreditSystem)
    return;

  this->_pCreditSystem->releaseBulkReferences(
      this->_references,
      this->_generations);

  this->_references.clear();
  this->_generations.clear();
}

bool CreditReferencer::isCreditReferenced(Credit credit) const noexcept {
  if (!this->_pCreditSystem)
    return false;

  if (this->_references.size() <= credit._id ||
      this->_generations[credit._id] != credit._generation) {
    return false;
  }

  return this->_references[credit._id] > 0;
}

} // namespace CesiumUtility
