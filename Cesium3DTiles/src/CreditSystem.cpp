#include "Cesium3DTiles/CreditSystem.h"

namespace Cesium3DTiles {

    Credit CreditSystem::createCredit(const std::string& html) {   
        // if this credit already exists, return a Credit handle to it
        for (Credit credit = 0; credit < _credits.size(); credit++) {
            if(_credits[credit].first == html) {
                return credit;
            }
        }

        // this is a new credit so add it to _credits
        _credits.push_back(std::make_pair(html, -1));

        // return a Credit handle to the newly created entry
        return _credits.size() - 1;
    }

    std::string CreditSystem::getHTML(Credit credit) const {
        if(credit < _credits.size()) {
            return std::string(_credits[credit].first);
        }
        return "Error: Invalid Credit object, cannot get HTML string.";
    }

    void CreditSystem::addCreditToFrame(Credit credit) {
        // if this credit has already been added to the current frame, there's nothing to do
        if (_credits[credit].second == _currentFrameNumber) {
            return;
        }

        // add the credit to this frame
        _creditsToShowThisFrame.push_back(credit);

        // if the credit was shown last frame, remove it from _creditsToNoLongerShowThisFrame since it will still be shown
        if (_credits[credit].second == _currentFrameNumber - 1) {
            for (size_t i = 0; i < _creditsToNoLongerShowThisFrame.size(); i++) {
                if (_creditsToNoLongerShowThisFrame[i] == credit) {
                    _creditsToNoLongerShowThisFrame.erase(_creditsToNoLongerShowThisFrame.begin() + int32_t(i));
                    break;
                }
            }
        }

        // update the last frame this credit was shown
        _credits[credit].second = _currentFrameNumber;
    }

    void CreditSystem::startNextFrame() {
        _creditsToNoLongerShowThisFrame.swap(_creditsToShowThisFrame);
        _creditsToShowThisFrame.clear();
        _currentFrameNumber++;
    }
}