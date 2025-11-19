#pragma once

#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Library.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CesiumUtility {

class CreditSystem;

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
    return this->_id == rhs._id && this->_generation == rhs._generation;
  }

private:
  int32_t _id;
  int32_t _generation;

  Credit(int32_t id, int32_t generation) noexcept
      : _id(id), _generation(generation) {}

  friend class CreditSystem;
  friend class CreditReferencer;
};

/**
 * @brief Represents a source of credits, such as a tileset or raster overlay,
 * provided to a \ref CreditSystem.
 *
 * While the \ref CreditSystem does not directly map credit source instances to
 * tilesets or raster overlays (or vice-versa), a tileset or raster overlay can
 * be queried for its credit source instance and that instance can be compared
 * against one known to the credit system.
 *
 * When the last reference to a credit source is released, all credits
 * associated with that source are invalidated as well.
 */
class CESIUMUTILITY_API CreditSource {
public:
  /**
   * @brief Constructs a new credit source associated with a given credit
   * system.
   *
   * @param creditSystem The credit system to associate with this source.
   */
  CreditSource(CreditSystem& creditSystem) noexcept;

  /**
   * @brief Constructs a new credit source associated with a given credit
   * system.
   *
   * @param pCreditSystem The credit system to associate with this source. This
   * is allowed to be nullptr in order to enable simpler client code when
   * working with an optional credit system.
   */
  CreditSource(const std::shared_ptr<CreditSystem>& pCreditSystem) noexcept;

  ~CreditSource() noexcept;

  /**
   * @brief Gets the \ref CreditSystem associated with this source.
   *
   * This may be nullptr if this source was never associated with a credit
   * system, or if the credit system has been destroyed.
   */
  CreditSystem* getCreditSystem() noexcept;

  /**
   * @copydoc getCreditSystem
   */
  const CreditSystem* getCreditSystem() const noexcept;

private:
  void notifyCreditSystemDestroyed() noexcept;

  CreditSystem* _pCreditSystem;

  friend class CreditSystem;
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
  ~CreditSystem() noexcept;

  /**
   * @brief Inserts a credit string.
   *
   * @param pSource The source of the credit. This should be an instance created
   * and owned by a tileset, raster overlay, or other data source.
   * @param html The HTML string for the credit.
   * @param showOnScreen Whether or not the credit should be shown on screen.
   * Credits not shown on the screen should be shown in a separate popup window.
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   */
  Credit createCredit(
      const CreditSource& source,
      std::string&& html,
      bool showOnScreen = false);

  /**
   * @brief Inserts a credit string.
   *
   * @param pSource The source of the credit. This should be an instance created
   * and owned by a tileset, raster overlay, or other data source.
   * @param html The HTML string for the credit.
   * @param showOnScreen Whether or not the credit should be shown on screen.
   * Credits not shown on the screen should be shown in a separate popup window.
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   */
  Credit createCredit(
      const CreditSource& source,
      const std::string& html,
      bool showOnScreen = false);

  /**
   * @brief Inserts a credit string.
   *
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   *
   * @deprecated Use the overload that takes a CreditSource pointer.
   */
  Credit createCredit(std::string&& html, bool showOnScreen = false);

  /**
   * @brief Inserts a credit string.
   *
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   *
   * @deprecated Use the overload that takes a CreditSource pointer.
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
   * @brief Get the HTML string for this credit.
   */
  const std::string& getHtml(Credit credit) const noexcept;

  /**
   * @brief Gets the source of this credit.
   *
   * @return The source of this credit, or nullptr if the credit is invalid or
   * was created by a \ref CreditSource that has been destroyed.
   */
  const CreditSource* getCreditSource(Credit credit) const noexcept;

  /**
   * @brief Adds a reference to a credit, incrementing its reference count. The
   * referenced credit will be shown until its reference count goes back down to
   * zero.
   *
   * @param credit The credit to reference.
   * @returns `true` if the credit was valid and the reference was added.
   * `false` if the credit was created by a \ref CreditSource that has been
   * destroyed and so the reference could not be added.
   */
  bool addCreditReference(Credit credit);

  /**
   * @brief Removes a reference from a credit, decrementing its reference count.
   * When the reference count goes to zero, this credit will no longer be shown.
   *
   * @param credit The credit from which to remove a reference.
   * @returns `true` if the credit was valid and the reference was removed.
   * `false` if the credit was created by a \ref CreditSource that has been
   * destroyed and so the reference could not be removed.
   */
  bool removeCreditReference(Credit credit);

  /**
   * @brief Gets a snapshot of the credits. The returned instance is only valid
   * until the next call to this method.
   *
   * The snapshot will include a sorted list of credits that are currently
   * active, as well as a list of credits that have been removed since the last
   * snapshot.
   */
  const CreditsSnapshot& getSnapshot() noexcept;

  /**
   * @brief Gets the default credit source used when no other source is
   * specified.
   *
   * @deprecated Instead of using the default, create a CreditSource instance
   * for each tileset, raster overlay, or other data source.
   */
  const CreditSource& getDefaultCreditSource() const noexcept;

private:
  void addBulkReferences(
      const std::vector<int32_t>& references,
      const std::vector<int32_t>& generations) noexcept;
  void releaseBulkReferences(
      const std::vector<int32_t>& references,
      const std::vector<int32_t>& generations) noexcept;

  void createCreditSource(CreditSource& creditSource) noexcept;
  void destroyCreditSource(CreditSource& creditSource) noexcept;

  const std::string INVALID_CREDIT_MESSAGE =
      "Error: Invalid Credit, cannot get HTML string.";

  struct CreditRecord {
    std::string html{};
    bool showOnScreen{false};
    int32_t referenceCount{0};
    bool shownLastSnapshot{0};
    int32_t generation{0};
    const CreditSource* pSource{nullptr};
  };

  std::vector<CreditSource*> _creditSources;
  std::vector<CreditRecord> _credits;
  std::vector<Credit> _creditsToNoLongerShowThisSnapshot;
  CreditsSnapshot _snapshot;

  // Each entry in this vector is an index into _credits that is unused and can
  // be reused for a new credit.
  std::vector<size_t> _unusedCreditRecords;

  CreditSource _defaultCreditSource{*this};

  friend class CreditReferencer;
  friend class CreditSource;
};
} // namespace CesiumUtility
