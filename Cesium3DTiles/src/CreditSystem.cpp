#include "Cesium3DTiles/CreditSystem.h"
#include <map>
#include <string>
#include <utility>

namespace {
    bool operator==(const aCesium3DTiles::Credit& lhs, const aCesium3DTiles::Credit& rhs) { return lhs.first == rhs.first };

    bool operator<(const aCesium3DTiles::Credit& lhs, const aCesium3DTiles::Credit& rhs) { return lhs.first < rhs.first };
}

namespace aCesium3DTiles {

    std::map<std::string, int> CreditSystem::creditToId = {};
    int CreditSystem::nextId = 0;

    Credit CreditSystem::createCredit(const std::string& html) {
        std::pair<std::map<std::string, int>::iterator, bool> insertionResult;
        insertionResult = creditToId.insert(std::pair<std::string, int>(html, nextId));
        
        /*
        int id;
        if (insertionResult.second) {
            // this is a new credit
            id = nextId++;
        } else {
            // this is an existing credit, so assign it the old id  
            id = insertionResult.first->second;
        }
        */
        return Credit(

            // the id
            insertionResult.second ?
                // this is a new credit
                nextId++ :
                // this is an existing credit, so assign it the old id
                insertionResult.first->second, 

            // the html string
            insertionResult.first->first
        ); 
    }

    void addCreditToFrame(const Credit credit) {
        creditsToNoLongerShowThisFrame.remove(credit);
        creditsToShowThisFrame.insert(credit).second;
    }

    void startNextFrame() {
        creditsToNoLongerShowThisFrame.clear();
        creditsToNoLongerShowThisFrame(creditsToShowThisFrame);
        creditsToShowThisFrame.clear();
    }
}