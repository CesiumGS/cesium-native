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
    struct CESIUM3DTILES_API Credit {
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
         * @param creditString the string representing this credit.
         * @param wrapStringInHtml whether to wrap the credit string into an html element. 
         *
         * @return If this string already exists, returns a Credit handle to the existing entry. 
         * Otherwise returns a Credit handle to a new entry.
         */
        Credit createCredit(const std::string& creditString, bool htmlEncode = true); 

        /**
         * @brief Get the HTML string for this credit
         */
        const std::string& getHtml(Credit credit) const;

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
        const std::string INVALID_CREDIT_MESSAGE = "Error: Invalid Credit, cannot get HTML string.";

        struct HtmlAndLastFrameNumber {
            std::string html;
            int32_t lastFrameNumber;
        };

        std::vector<HtmlAndLastFrameNumber> _credits;

        int32_t _currentFrameNumber = 0;
        std::vector<Credit> _creditsToShowThisFrame;
        std::vector<Credit> _creditsToNoLongerShowThisFrame;
    };
}