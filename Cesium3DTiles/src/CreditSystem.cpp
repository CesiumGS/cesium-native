#include "Cesium3DTiles/CreditSystem.h"
#include <map>
#include <set>
#include <string>
#include <utility>

namespace Cesium3DTiles {

    Credit CreditSystem::createCredit(const std::string& html) {
        std::pair<std::map<std::string, int>::iterator, bool> insertionResult;
        insertionResult = creditToId.insert(std::pair<std::string, int>(html, nextId));
        
        return Credit {

            // the id
            insertionResult.second ?
                // this is a new credit
                nextId++ :
                // this is an existing credit, so assign it the old id
                insertionResult.first->second, 

            // the html string
            insertionResult.first->first
        }; 
    }

    void CreditSystem::addCreditToFrame(const Credit credit) {
        creditsToNoLongerShowThisFrame.erase(credit);
        creditsToShowThisFrame.insert(credit).second;
    }

    void CreditSystem::startNextFrame() {
        creditsToNoLongerShowThisFrame.clear();
        creditsToNoLongerShowThisFrame = std::set(creditsToShowThisFrame);
        creditsToShowThisFrame.clear();
    }
}