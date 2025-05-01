#pragma once

#include <CesiumUtility/Library.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CesiumUtility {

/**
 * @brief Represents an HTML string that should be shown on screen to attribute
 * third parties for used data, imagery, etc. Acts as a handle into a
 * {@link CreditSystem} object that actually holds the credit string.
 */
struct CESIUMUTILITY_API Credit {
public:
  /**
   * @brief Returns `true` if two credit objects have the same ID.
   */
  bool operator==(const Credit& rhs) const noexcept {
    return this->id == rhs.id;
  }

private:
  size_t id;

  Credit(size_t id_) noexcept : id(id_) {}

  friend class CreditSystem;
  friend class CreditReferencer;
};

/**
 * @brief A snapshot of the credits currently active in a {@link CreditSystem}.
 */
struct CreditsSnapshot {
  /**
   * @brief The credits that are currently active.
   */
  std::vector<Credit> currentCredits;

  /**
   * @brief The credits that were removed since the last call to
   * {@link CreditSystem::getSnapshot}.
   */
  std::vector<Credit> removedCredits;
};

/**
 * @brief Creates and manages {@link Credit} objects. Avoids repetitions and
 * tracks which credits should be shown and which credits should be removed this
 * frame.
 */
class CESIUMUTILITY_API CreditSystem final {
public:
  /**
   * @brief Inserts a credit string
   *
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   */
  Credit createCredit(std::string&& html, bool showOnScreen = false);

  /**
   * @brief Inserts a credit string
   *
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   */
  Credit createCredit(const std::string& html, bool showOnScreen = false);

  /**
   * @brief Gets whether or not the credit should be shown on screen.
   */
  bool shouldBeShownOnScreen(Credit credit) const noexcept;

  /**
   * @brief Sets whether or not the credit should be shown on screen.
   */
  void setShowOnScreen(Credit credit, bool showOnScreen) noexcept;

  /**
   * @brief Get the HTML string for this credit
   */
  const std::string& getHtml(Credit credit) const noexcept;

  /**
   * @brief Adds a reference to a credit, incrementing its reference count. The
   * referenced credit will be shown until its reference count goes back down to
   * zero.
   *
   * @param credit The credit to reference.
   */
  void addCreditReference(Credit credit);

  /**
   * @brief Removes a reference from a credit, decrementing its reference count.
   * When the reference count goes to zero, this credit will no longer be shown.
   *
   * @param credit The credit from which to remove a reference.
   */
  void removeCreditReference(Credit credit);

  /**
   * @brief Gets a snapshot of the credits. The returned instance is only valid
   * until the next call to this method.
   *
   * The snapshot will include a sorted list of credits that are currently
   * active, as well as a list of credits that have been removed since the last
   * snapshot.
   */
  const CreditsSnapshot& getSnapshot() noexcept;

private:
  void addBulkReferences(const std::vector<int32_t>& references) noexcept;
  void releaseBulkReferences(const std::vector<int32_t>& references) noexcept;

  const std::string INVALID_CREDIT_MESSAGE =
      "Error: Invalid Credit, cannot get HTML string.";

  struct CreditRecord {
    std::string html;
    bool showOnScreen;
    int32_t referenceCount;
    bool shownLastSnapshot;
  };

  std::vector<CreditRecord> _credits;
  std::vector<Credit> _creditsToNoLongerShowThisSnapshot;
  CreditsSnapshot _snapshot;

  friend class CreditReferencer;
};
} // namespace CesiumUtility
