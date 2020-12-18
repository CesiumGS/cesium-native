#pragma once

#include "Cesium3DTiles/Library.h"
//#include "Cesium3DTiles/Credit.h"
#include <map>
#include <string>
#include <utility>

namespace aCesium3DTiles {

    typedef std::pair<int, const std::string&> Credit;

    class CESIUM3DTILES_API CreditSystem final {
    public:

        /**
         * @brief Constructs a new instance.
         */
        CreditSystem()
        {}

        /**
         * @brief Destroys this instance.
         */
        ~CreditSystem()
        {}

        /**
         * @brief Inserts a credit string
         *
         * @return If this string already exists, returns a Credit handle to the existing entry. 
         * Otherwise returns a Credit handle to a new entry.
         */
        Credit createCredit(const std::string& html); 

        /**
         * @brief Adds the Credit to the set of credits to show this frame
         */
        void addCreditToFrame(const Credit credit);

        /**
         * @brief Notifies this CreditSystem to start tracking the credits to show for the next frame.
         */
        void startNextFrame();

    private:

        // indexed html strings and their unique IDs to efficiently check if this is an existing credit 
        std::map<std::string, int> creditToId;
        // the ID to assign the next new credit  
        int nextId;

        std::set<Credit> creditsToShowThisFrame;
        std::set<Credit> creditsToNoLongerShowThisFrame;
    };
}