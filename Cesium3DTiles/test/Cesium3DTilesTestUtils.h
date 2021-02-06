#pragma once

#include <chrono>
#include <string>

#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/ViewState.h"
#include "Cesium3DTiles/ViewUpdateResult.h"

namespace Cesium3DTilesTests 
{
    /**
     * @brief Creates a default ViewState.
     * 
     * The configuration of this ViewState is not specified.
     * 
     * @param p The position
     * @param d The direction
     * @return The ViewState
     */
    Cesium3DTiles::ViewState createViewState(const glm::dvec3& p, const glm::dvec3 d);

    /**
     * @brief Creates a default ViewState.
     * 
     * The configuration of this ViewState is not specified.
     * 
     * @return The ViewState
     */
    Cesium3DTiles::ViewState createViewState();

    /**
     * @brief Returns whether the given string ends with the given suffix.
     * 
     * If the string is shorter than the suffix, then `false` is returned.
     * @param s The string
     * @param s The suffix
     * @return The result
     */
    bool endsWith(const std::string& s, const std::string& suffix);

    /**
     * @brief Sleeps (blocks the calling thread) for the given number of milliseconds.
     * 
     * If the given number is not positive, then this call will not block.
     * 
     * @param ms The number of milliseconds
     */
    void sleepMsLogged(int32_t ms);

    /**
     * @brief Returns a formatted string summary of the given ViewUpdateResult.
     * 
     * The exact format is not specified. It should be easy to read, though...
     * 
     * @param r The ViewUpdateResult
     * @return The string
     */
    std::string viewUpdateResultToString(const Cesium3DTiles::ViewUpdateResult& r);

    /**
     * @brief Print a string representation of the given ViewUpdateResult
     * 
     * As created with `viewUpdateResultToString`
     * 
     * @param r The ViewUpdateResult
     */
    void printViewUpdateResult(const Cesium3DTiles::ViewUpdateResult& r);

    /**
     * @brief Create a short string for the given TileID
     * 
     * The exact format is not specified.
     * 
     * @param tileId The tile ID
     * @return The string
     */
    std::string createTileIdString(const Cesium3DTiles::TileID& tileId);
}
