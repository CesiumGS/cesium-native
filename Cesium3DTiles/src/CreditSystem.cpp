#include "Cesium3DTiles/CreditSystem.h"

namespace Cesium3DTiles {

    Credit CreditSystem::createCredit(const std::string& html) {   
        // if this credit already exists, return a Credit handle to it
        for (size_t id = 0; id < _credits.size(); id++) {
            if(_credits[id].first == html) {
                return Credit(id);
            }
        }

        // this is a new credit so add it to _credits
        _credits.push_back(std::make_pair(html, -1));

        // return a Credit handle to the newly created entry
        return Credit(_credits.size() - 1);
    }

    std::string CreditSystem::getHTML(Credit credit) const {
        if(credit.id < _credits.size()) {
            return std::string(_credits[credit.id].first);
        }
        return "Error: Invalid Credit object, cannot get HTML string.";
    }

    void CreditSystem::addCreditToFrame(Credit credit) {
        // if this credit has already been added to the current frame, there's nothing to do
        if (_credits[credit.id].second == _currentFrameNumber) {
            return;
        }

        // add the credit to this frame
        _creditsToShowThisFrame.push_back(credit);

        // if the credit was shown last frame, remove it from _creditsToNoLongerShowThisFrame since it will still be shown
        if (_credits[credit.id].second == _currentFrameNumber - 1) {
            for (size_t i = 0; i < _creditsToNoLongerShowThisFrame.size(); i++) {
                if (_creditsToNoLongerShowThisFrame[i] == credit) {
                    _creditsToNoLongerShowThisFrame.erase(_creditsToNoLongerShowThisFrame.begin() + int32_t(i));
                    break;
                }
            }
        }

        // update the last frame this credit was shown
        _credits[credit.id].second = _currentFrameNumber;
    }

    void CreditSystem::startNextFrame() {
        _creditsToNoLongerShowThisFrame.swap(_creditsToShowThisFrame);
        _creditsToShowThisFrame.clear();
        _currentFrameNumber++;
    }
}