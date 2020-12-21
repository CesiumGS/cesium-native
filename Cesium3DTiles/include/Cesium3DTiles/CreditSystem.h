#pragma once

#include "Cesium3DTiles/Library.h"
#include <map>
#include <set>
#include <string>
#include <utility>

namespace Cesium3DTiles {

    /**
     * @brief Handle into a {@link CreditSystem} representing a html credit string.
     */
    struct Credit final {
        const int id;
        const std::string& html;

        bool operator==(const Credit& rhs) const { return this->id == rhs.id; }

        bool operator<(const Credit& rhs) const { return this->id < rhs.id; }
    };

    /**
     * @brief Creates and manages {@link Credit} objects. Avoids repetitions and
     * tracks which credits should be shown and which credits should be removed this frame.
     */
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

        /**
         * @brief Get the credits to show this frame.
         */
         const std::set<Credit>& getCreditsToShowThisFrame() const { return creditsToShowThisFrame; }

         /**
          * @brief Get the credits that were shown last frame but should no longer be shown.
          */
         const std::set<Credit>& getCreditsToNoLongerShowThisFrame() const { return creditsToNoLongerShowThisFrame; }

    private:

        // indexed html strings and their unique IDs to efficiently check if this is an existing credit 
        std::map<std::string, int> creditToId;
        // the ID to assign the next new credit  
        int nextId;

        std::set<Credit> creditsToShowThisFrame;
        std::set<Credit> creditsToNoLongerShowThisFrame;
    };
}