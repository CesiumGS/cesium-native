#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace CesiumUtility {

class CreditSystem;
struct Credit;

/**
 * @brief Provides an easy way to reference a set of credits in a
 * {@link CreditSystem} so that the references can be easily released later.
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
  ~CreditReferencer() noexcept;

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
