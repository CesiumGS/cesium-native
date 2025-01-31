#pragma once

#include <CesiumUtility/Library.h>

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
   * @brief Adds the Credit to the set of credits to show this frame
   */
  void addCreditToFrame(Credit credit);

  /**
   * @brief Notifies this CreditSystem to start tracking the credits to show for
   * the next frame.
   */
  void startNextFrame() noexcept;

  /**
   * @brief Get the credits to show this frame.
   */
  const std::vector<Credit>& getCreditsToShowThisFrame() noexcept;

  /**
   * @brief Get the credits that were shown last frame but should no longer be
   * shown.
   */
  const std::vector<Credit>&
  getCreditsToNoLongerShowThisFrame() const noexcept {
    return _creditsToNoLongerShowThisFrame;
  }

private:
  const std::string INVALID_CREDIT_MESSAGE =
      "Error: Invalid Credit, cannot get HTML string.";

  struct HtmlAndLastFrameNumber {
    std::string html;
    bool showOnScreen;
    int32_t lastFrameNumber;
    int count;
  };

  std::vector<HtmlAndLastFrameNumber> _credits;

  int32_t _currentFrameNumber = 0;
  std::vector<Credit> _creditsToShowThisFrame;
  std::vector<Credit> _creditsToNoLongerShowThisFrame;
};
} // namespace CesiumUtility
