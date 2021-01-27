#include "Cesium3DTiles/CreditSystem.h"
#include <algorithm>

namespace Cesium3DTiles {

    Credit CreditSystem::createCredit(const std::string& creditString, bool wrapStringInHtml) {   
        std::string html = wrapStringInHtml ? 
            "<div style=\"color:white\">" + creditString + "</div>" :
            creditString;

        // if this credit already exists, return a Credit handle to it
        for (size_t id = 0; id < _credits.size(); ++id) {
            if(_credits[id].html == html) {
                return Credit(id);
            }
        }

        // this is a new credit so add it to _credits
        _credits.push_back({html, -1});

        // return a Credit handle to the newly created entry
        return Credit(_credits.size() - 1);
    }

    const std::string& CreditSystem::getHtml(Credit credit) const {
        if(credit.id < _credits.size()) {
            return _credits[credit.id].html;
        }
        return INVALID_CREDIT_MESSAGE;
    }

    void CreditSystem::addCreditToFrame(Credit credit) {
        // if this credit has already been added to the current frame, there's nothing to do
        if (_credits[credit.id].lastFrameNumber == _currentFrameNumber) {
            return;
        }

        // add the credit to this frame
        _creditsToShowThisFrame.push_back(credit);

        // if the credit was shown last frame, remove it from _creditsToNoLongerShowThisFrame since it will still be shown
        if (_credits[credit.id].lastFrameNumber == _currentFrameNumber - 1) {
            _creditsToNoLongerShowThisFrame.erase(
                std::remove(
                    _creditsToNoLongerShowThisFrame.begin(),
                    _creditsToNoLongerShowThisFrame.end(),
                    credit
                ),
                _creditsToNoLongerShowThisFrame.end()
            );
        }

        // update the last frame this credit was shown
        _credits[credit.id].lastFrameNumber = _currentFrameNumber;
    }

    void CreditSystem::startNextFrame() {
        _creditsToNoLongerShowThisFrame.swap(_creditsToShowThisFrame);
        _creditsToShowThisFrame.clear();
        _currentFrameNumber++;
    }

    std::string CreditSystem::getHtmlPageToShowThisFrame() const {
        std::string htmlPage = "<body>\n";
        for (size_t i = 0; i < _creditsToShowThisFrame.size(); ++i) {
            htmlPage += this->getHtml(_creditsToShowThisFrame[i]) + "\n";
        }
        htmlPage += "</body>";
        return htmlPage;
    }
}