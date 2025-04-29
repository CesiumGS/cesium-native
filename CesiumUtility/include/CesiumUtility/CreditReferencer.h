#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace CesiumUtility {

class CreditSystem;
struct Credit;

/**
 * @brief Provides a way to reference a set of credits in a
 * {@link CreditSystem} so that the references can easily be released later.
 *
 * Multiple CreditReferencers may be used on the same credit system to track
 * separate sets of references -- e.g., two sets of credits from different
 * frames.
 */
class CreditReferencer {
public:
  /**
   * @brief Constructs a new credit referencer without a credit system.
   *
   * The methods on this instance will have no effect before the credit system
   * is set by calling {@link setCreditSystem}.
   */
  CreditReferencer() noexcept;

  /**
   * @brief Constructs a new credit referencer.
   *
   * @param pCreditSystem The credit system that owns the referenced credits.
   * This may be nullptr, but methods on this instance will have no effect
   * before the credit system is set.
   */
  CreditReferencer(const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept;

  /**
   * @brief Copies an instance, increasing the credit references on the
   * {@link CreditSystem} accordingly.
   *
   * @param rhs The instance to copy.
   */
  CreditReferencer(const CreditReferencer& rhs) noexcept;

  /**
   * @brief Moves an instance. After the move, the `rhs` will not own any
   * references, so this operation does not affect the total number of credit
   * references on the {@link CreditSystem}.
   *
   * @param rhs The instance to move.
   */
  CreditReferencer(CreditReferencer&& rhs) noexcept;

  /**
   * @brief Destroys this instance, releasing all of its references.
   */
  ~CreditReferencer() noexcept;

  /**
   * @brief Copy-assigns another instance to this one. The `rhs` references are
   * copied to this instance and then all credit references previously owned by
   * this instance are released.
   *
   * @param rhs The instance to copy.
   * @returns A reference to this instance.
   */
  CreditReferencer& operator=(const CreditReferencer& rhs) noexcept;

  /**
   * @brief Move-assigns another instance to this one. The `rhs` references are
   * moved to this instance and then all credit references previously owned by
   * this instance are released.
   *
   * @param rhs The instance to move.
   * @return A reference to this instance.
   */
  CreditReferencer& operator=(CreditReferencer&& rhs) noexcept;

  /**
   * @brief Gets the credit system that this instance references.
   */
  const std::shared_ptr<CreditSystem>& getCreditSystem() const noexcept;

  /**
   * @brief Sets the credit system that this instance references.
   *
   * If the specified credit system is different from the current one, this
   * method will clear all current credit references.
   *
   * @param pCreditSystem The new credit system.
   */
  void
  setCreditSystem(const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept;

  /**
   * @brief Adds a reference to a credit.
   *
   * @param credit The credit to reference.
   */
  void addCreditReference(Credit credit) noexcept;

  /**
   * @brief Releases all references that have been added to this instance.
   */
  void releaseAllReferences() noexcept;

private:
  std::shared_ptr<CreditSystem> _pCreditSystem;
  std::vector<int32_t> _references;
};

} // namespace CesiumUtility
