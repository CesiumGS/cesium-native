#include <CesiumUtility/CreditSystem.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace CesiumUtility {

Credit CreditSystem::createCredit(const std::string& html, bool showOnScreen) {
  return this->createCredit(std::string(html), showOnScreen);
}

Credit CreditSystem::createCredit(std::string&& html, bool showOnScreen) {
  // if this credit already exists, return a Credit handle to it
  for (size_t id = 0; id < _credits.size(); ++id) {
    if (_credits[id].html == html) {
      // Override the existing credit's showOnScreen value.
      _credits[id].showOnScreen = showOnScreen;
      return Credit(id);
    }
  }

  _credits.push_back({std::move(html), showOnScreen, -1, 0});

  return Credit(_credits.size() - 1);
}

bool CreditSystem::shouldBeShownOnScreen(Credit credit) const noexcept {
  if (credit.id < _credits.size()) {
    return _credits[credit.id].showOnScreen;
  }
  return false;
}

void CreditSystem::setShowOnScreen(Credit credit, bool showOnScreen) noexcept {
  if (credit.id < _credits.size()) {
    _credits[credit.id].showOnScreen = showOnScreen;
  }
}

const std::string& CreditSystem::getHtml(Credit credit) const noexcept {
  if (credit.id < _credits.size()) {
    return _credits[credit.id].html;
  }
  return INVALID_CREDIT_MESSAGE;
}

void CreditSystem::addCreditToFrame(Credit credit) {
  // if this credit has already been added to the current frame, there's nothing
  // to do
  if (_credits[credit.id].lastFrameNumber == _currentFrameNumber) {
    ++_credits[credit.id].count;
    return;
  }

  // add the credit to this frame
  _creditsToShowThisFrame.push_back(credit);

  // if the credit was shown last frame, remove it from
  // _creditsToNoLongerShowThisFrame since it will still be shown
  if (_credits[credit.id].lastFrameNumber == _currentFrameNumber - 1) {
    _creditsToNoLongerShowThisFrame.erase(
        std::remove(
            _creditsToNoLongerShowThisFrame.begin(),
            _creditsToNoLongerShowThisFrame.end(),
            credit),
        _creditsToNoLongerShowThisFrame.end());
  }

  // update the last frame this credit was shown
  _credits[credit.id].lastFrameNumber = _currentFrameNumber;
}

void CreditSystem::startNextFrame() noexcept {
  _creditsToNoLongerShowThisFrame.swap(_creditsToShowThisFrame);
  _creditsToShowThisFrame.clear();
  _currentFrameNumber++;
  for (const auto& credit : _creditsToNoLongerShowThisFrame) {
    _credits[credit.id].count = 0;
  }
}

const std::vector<Credit>& CreditSystem::getCreditsToShowThisFrame() noexcept {
  // sort credits based on the number of occurrences
  if (_creditsToShowThisFrame.size() < 2) {
    return _creditsToShowThisFrame;
  }
  std::sort(
      _creditsToShowThisFrame.begin(),
      _creditsToShowThisFrame.end(),
      [this](const Credit& a, const Credit& b) {
        int32_t aCounts = _credits[a.id].count;
        int32_t bCounts = _credits[b.id].count;
        if (aCounts == bCounts)
          return a.id < b.id;
        else
          return aCounts > bCounts;
      });
  return _creditsToShowThisFrame;
}
} // namespace CesiumUtility
