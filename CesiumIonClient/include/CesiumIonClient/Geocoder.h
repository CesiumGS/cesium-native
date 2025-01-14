#pragma once

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>

#include <string>
#include <variant>
#include <vector>

namespace CesiumIonClient {

/**
 * @brief The supported types of requests to geocoding API.
 */
enum GeocoderRequestType {
  /**
   * @brief Perform a full search from a complete query.
   */
  Search,

  /**
   * @brief Perform a quick search based on partial input, such as while a user
   * is typing.
   * The search results may be less accurate or exhaustive than using {@link GeocoderRequestType::Search}.
   */
  Autocomplete
};

/**
 * @brief The supported providers that can be accessed through ion's geocoder
 * API.
 */
enum GeocoderProviderType {
  /**
   * @brief Google geocoder, for use with Google data.
   */
  Google,

  /**
   * @brief Bing geocoder, for use with Bing data.
   */
  Bing,

  /**
   * @brief Use the default geocoder as set on the server. Used when neither
   * Bing or Google data is used.
   */
  Default
};

/**
 * @brief A single feature (a location or region) obtained from a geocoder
 * service.
 */
struct GeocoderFeature {
  /**
   * @brief The user-friendly display name of this feature.
   */
  std::string displayName;

  /**
   * @brief The region on the globe for this feature.
   */
  std::variant<CesiumGeospatial::GlobeRectangle, CesiumGeospatial::Cartographic>
      destination;

  /**
   * @brief Returns a {@link CesiumGeospatial::GlobeRectangle} representing this feature.
   *
   * If the geocoder service returned a bounding box for this result, this will
   * return the bounding box. If the geocoder service returned a coordinate for
   * this result, this will return a zero-width rectangle at that coordinate.
   */
  CesiumGeospatial::GlobeRectangle getGlobeRectangle() const;

  /**
   * @brief Returns a {@link CesiumGeospatial::Cartographic} representing this feature.
   *
   * If the geocoder service returned a bounding box for this result, this will
   * return the center of the bounding box. If the geocoder service returned a
   * coordinate for this result, this will return the coordinate.
   */
  CesiumGeospatial::Cartographic getCartographic() const;
};

/**
 * @brief Attribution information for a query to a geocoder service.
 */
struct GeocoderAttribution {
  /**
   * @brief An HTML string containing the necessary attribution information.
   */
  std::string html;

  /**
   * @brief If true, the credit should be visible in the main credit container.
   * Otherwise, it can appear in a popover.
   */
  bool showOnScreen;
};

/**
 * @brief The result of making a request to a geocoder service.
 */
struct GeocoderResult {
  /**
   * @brief Any necessary attributions for this geocoder result.
   */
  std::vector<GeocoderAttribution> attributions;

  /**
   * @brief The features obtained from this geocoder service, if any.
   */
  std::vector<GeocoderFeature> features;
};

}; // namespace CesiumIonClient