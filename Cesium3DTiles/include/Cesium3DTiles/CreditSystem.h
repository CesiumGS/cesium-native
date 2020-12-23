#pragma once

#include "Cesium3DTiles/Library.h"
#include <vector>
#include <string>
#include <utility>

namespace Cesium3DTiles {

    /**
     * @brief Represents an HTML string that should be shown on screen to attribute third parties for used data, imagery, etc.  
     * Acts as a handle into a {@link CreditSystem} object that actually holds the credit string. 
     */
    struct Credit {
        public:
            bool operator==(const Credit& rhs) const { return this->id == rhs.id; }

            bool operator<(const Credit& rhs) const { return this->id < rhs.id; }

        private:
            size_t id;

            Credit(size_t id_) {
                id = id_;
            }
            
            friend class CreditSystem;
    };

    /**
     * @brief Creates and manages {@link Credit} objects. Avoids repetitions and
     * tracks which credits should be shown and which credits should be removed this frame.
     */
    class CESIUM3DTILES_API CreditSystem final {
    public:
        /**
         * @brief Inserts a credit string
         *
         * @return If this string already exists, returns a Credit handle to the existing entry. 
         * Otherwise returns a Credit handle to a new entry.
         */
        Credit createCredit(const std::string& html); 

        /**
         * @brief Get the HTML string for this credit
         */
        std::string getHTML(Credit credit) const;

        /**
         * @brief Adds the Credit to the set of credits to show this frame
         */
        void addCreditToFrame(Credit credit);

        /**
         * @brief Notifies this CreditSystem to start tracking the credits to show for the next frame.
         */
        void startNextFrame();

        /**
         * @brief Get the credits to show this frame.
         */
         const std::vector<Credit>& getCreditsToShowThisFrame() const { return _creditsToShowThisFrame; }

         /**
          * @brief Get the credits that were shown last frame but should no longer be shown.
          */
         const std::vector<Credit>& getCreditsToNoLongerShowThisFrame() const { return _creditsToNoLongerShowThisFrame; }

    private:
        // pairs of html strings and the frames they were last shown representing unique credits
        std::vector<std::pair<std::string, int32_t>> _credits;

        int32_t _currentFrameNumber = 0;
        std::vector<Credit> _creditsToShowThisFrame;
        std::vector<Credit> _creditsToNoLongerShowThisFrame;
    };
}