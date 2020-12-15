#pragma once

#include <map>
#include <vector>
#include <string>
#include "CesiumGeospatial/GlobeRectangle.h"

namespace Cesium3DTiles {

    class /*CESIUM3DTILES_API*/ Credit final {
    public:

        /**
         * @brief A bounding rectangle and zoom range within the imagery that should be attributed to this credit. 
         */
        struct CoverageArea final {
            CesiumGeospatial::GlobeRectangle rectangle;
            int zoomMin;
            int zoomMax;
        };

        /**
         * @brief Comparator for Credit objects to use with std::set. 
         */
        struct Comparator final {
            bool operator()(const int lhs, const int rhs) {
                return lhs < rhs;
            }
            
            bool operator()(const Credit& lhs, const Credit& rhs) {
                return lhs._id < rhs._id;
            }
        };

        /**
         * @brief Constructs a new instance. 
         *
         * @param html The HTML string this credit refers to.
         * @param showOnScreen Whether or not to show this credit on screen.
         */
        Credit(std::string html, std::vector<CoverageArea>& coverageAreas, bool showOnScreen);

        /**
         * @brief Destroys this instance.
         */
        ~Credit();

        /**
         * @brief Get the HTML string representing this Credit object.
         *
         * @returns The HTML string representing this Credit object. 
         */
        std::string getHTML() const { return std::string(_html); }

        /**
         * @brief Check whether this Credit should be shown on the screen.
         *
         * @returns The bool of whether this Credit should be shown on the screen. 
         */
        bool getShowOnScreen() const { return _showOnScreen; } 

        /**
         * @brief Check if the given rectangle and zoom is within any of the coverage areas for this credit. 
         * 
         * @param rectangle The rectangle to check against the coverage areas.
         * @param zoomLevel The level of zoom of the rectangle.
         * 
         * @returns A bool of whether the given rectangle and zoom is within the coverage areas.
         */
        bool withinCoverage(CesiumGeospatial::GlobeRectangle rectangle, int zoomLevel) const;

        /**
         * @brief Checks this and another Credit object for equality of id.
         *
         * @param rhs The Credit object to compare to this.
         *
         * @returns A bool of whether the two Credit objects are equal.
         */
        bool operator==(const Credit& rhs) const { return _id == rhs._id; }

        /**
         * @brief Ordering by id on Credit objects.
         *
         * @param rhs The Credit object to compare to this.
         *
         * @returns A bool of whether this is less than the given Credit object.
         */
        bool operator<(const Credit& rhs) const { return _id < rhs._id; }

    private:

        // map from html string to unique ID to efficiently check if this is an existing credit 
        static std::map<std::string, int> creditToId;
        // the ID to assign the next new credit  
        static int nextId;

        // the unique identifier for the HTML string this Credit object represents 
        int _id;
        // pointer to the html string
        char* _html;
        // whether to show this credit on screen
        bool _showOnScreen;
        // the bounding boxes covered by this credit
        std::vector<CoverageArea> _coverageAreas;
    };
}