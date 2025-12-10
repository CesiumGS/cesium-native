#pragma once

#include <CesiumUtility/Library.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CesiumUtility {

class CreditSystem;

/**
 * @brief Specifies how credit system snapshots should handle multiple
 * credits with the same HTML string.
 */
enum class CreditFilteringMode : uint8_t {
  /**
   * @brief No filtering is performed. Each unique @ref Credit is reported.
   */
  None = 0,

  /**
   * @brief Credits are filtered so that each reported credit has a combination
   * of HTML string and @ref CreditSystem::shouldBeShownOnScreen value that is
   * unique from all the other reported credits.
   *
   * If multiple credits have the same HTML string but different
   * @ref CreditSystem::shouldBeShownOnScreen values, they will be reported as
   * separate credits.
   *
   * It is unspecified which of the multiple credits with the same properties
   * will be reported.
   */
  UniqueHtmlAndShowOnScreen = 1,

  /**
   * @brief Credits with identical HTML strings are reported as one Credit even
   * if they have a different @ref CreditSource or @ref
   * CreditSystem::shouldBeShownOnScreen value.
   *
   * It is unspecified which of the multiple credits with the same source will
   * be reported. However, it is guaranteed that if any of the multiple credits
   * has @ref CreditSystem::shouldBeShownOnScreen set to `true`, the reported
   * credit will also have it set to `true`.
   */
  UniqueHtml = 2
};

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
  uint32_t _id;
  uint32_t _generation;

  Credit(uint32_t id, uint32_t generation) noexcept
      : _id(id), _generation(generation) {}

  friend class CreditSystem;
  friend class CreditReferencer;
};

/**
 * @brief Represents a source of credits, such as a tileset or raster overlay,
 * provided to a @ref CreditSystem.
 *
 * While the @ref CreditSystem does not directly map credit source instances to
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
   * @brief Gets the @ref CreditSystem associated with this source.
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
  /**
   * @brief Constructs a new instance.
   */
  CreditSystem() noexcept = default;
  ~CreditSystem() noexcept;

  CreditSystem(const CreditSystem&) = delete;
  CreditSystem& operator=(const CreditSystem&) = delete;

  /**
   * @brief Inserts a credit string.
   *
   * @param source The source of the credit. This should be an instance created
   * and owned by a tileset, raster overlay, or other data source.
   * @param html The HTML string for the credit.
   * @param showOnScreen Whether or not the credit should be shown on screen.
   * Credits not shown on the screen should be shown in a separate popup window.
   * @return If this string already exists from the same source, returns a
   * Credit handle to the existing entry. Otherwise returns a Credit handle to a
   * new entry.
   */
  Credit createCredit(
      const CreditSource& source,
      std::string&& html,
      bool showOnScreen = false);

  /** @copydoc createCredit */
  Credit createCredit(
      const CreditSource& source,
      const std::string& html,
      bool showOnScreen = false);

  /**
   * @brief Inserts a credit string associated with the @ref
   * getDefaultCreditSource.
   *
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   *
   * @deprecated Use the overload that takes a @ref CreditSource.
   */
  Credit createCredit(std::string&& html, bool showOnScreen = false);

  /**
   * @brief Inserts a credit string associated with the @ref
   * getDefaultCreditSource.
   *
   * @return If this string already exists, returns a Credit handle to the
   * existing entry. Otherwise returns a Credit handle to a new entry.
   *
   * @deprecated Use the overload that takes a @ref CreditSource.
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
   * `false` if the credit was created by a @ref CreditSource that has been
   * destroyed and so the reference could not be added.
   */
  bool addCreditReference(Credit credit);

  /**
   * @brief Removes a reference from a credit, decrementing its reference count.
   * When the reference count goes to zero, this credit will no longer be shown.
   *
   * @param credit The credit from which to remove a reference.
   * @returns `true` if the credit was valid and the reference was removed.
   * `false` if the credit was created by a @ref CreditSource that has been
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
   *
   * @param filteringMode Specifies how multiple credits with the same HTML
   * string should be reported in the snapshot.
   */
  const CreditsSnapshot& getSnapshot(
      CreditFilteringMode filteringMode =
          CreditFilteringMode::UniqueHtml) noexcept;

  /**
   * @brief Gets the default credit source used when no other source is
   * specified.
   *
   * @deprecated Instead of using the default, create a CreditSource instance
   * for each tileset, raster overlay, or other data source.
   */
  const CreditSource& getDefaultCreditSource() const noexcept;

private:
  struct CreditRecord {
    std::string html{};
    bool showOnScreen{false};
    int32_t referenceCount{0};
    bool shownLastSnapshot{0};
    uint32_t generation{0};
    const CreditSource* pSource{nullptr};
    uint32_t previousCreditWithSameHtml{INVALID_CREDIT_INDEX};
    uint32_t nextCreditWithSameHtml{INVALID_CREDIT_INDEX};
  };

  void addBulkReferences(
      const std::vector<int32_t>& references,
      const std::vector<uint32_t>& generations) noexcept;
  void releaseBulkReferences(
      const std::vector<int32_t>& references,
      const std::vector<uint32_t>& generations) noexcept;

  void createCreditSource(CreditSource& creditSource) noexcept;
  void destroyCreditSource(CreditSource& creditSource) noexcept;

  uint32_t filterCreditForSnapshot(
      CreditFilteringMode filteringMode,
      const CreditRecord& record) noexcept;

  const std::string INVALID_CREDIT_MESSAGE =
      "Error: Invalid Credit, cannot get HTML string.";

  static const uint32_t INVALID_CREDIT_INDEX{
      std::numeric_limits<uint32_t>::max()};

  std::vector<CreditSource*> _creditSources;
  std::vector<CreditRecord> _credits;
  CreditsSnapshot _snapshot;
  std::vector<int32_t> _referenceCountScratch;

  // These are credits that were shown in the last snapshot but whose
  // CreditSources have since been destroyed. They need to be reported in
  // removedCredits in the next snapshot.
  std::vector<Credit> _shownCreditsDestroyed;

  // Each entry in this vector is an index into _credits that is unused and can
  // be reused for a new credit.
  std::vector<size_t> _unusedCreditRecords;

  CreditSource _defaultCreditSource{*this};

  friend class CreditReferencer;
  friend class CreditSource;
};
} // namespace CesiumUtility
