#pragma once

#include <CesiumGeospatial/GlobeRectangle.h>

#include <cstdint>
#include <string>

namespace CesiumITwinClient {
/**
 * @brief Indicates the nature of the reality data.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/rm-rd-details/#classification
 * for more information.
 */
enum class ITwinRealityDataClassification : uint8_t {
  Unknown = 0,
  Terrain = 1,
  Imagery = 2,
  Pinned = 3,
  Model = 4,
  Undefined = 5
};

/**
 * @brief Obtains an \ref ITwinRealityDataClassification value from the provided
 * string.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/rm-rd-details/#classification
 * for a list of possible values.
 */
ITwinRealityDataClassification
iTwinRealityDataClassificationFromString(const std::string& str);

/**
 * @brief Information on reality data.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/operations/get-all-reality-data/#reality-data-metadata
 * for more information.
 */
struct ITwinRealityData {
  /**
   * @brief Identifier of the reality data.
   *
   * This identifier is assigned by the service at the creation of the reality
   * data. It is also unique.
   */
  std::string id;
  /**
   * @brief The name of the reality data.
   *
   * This property may not contain any control sequence such as a URL or code.
   */
  std::string displayName;
  /**
   * @brief A textual description of the reality data.
   *
   * This property may not contain any control sequence such as a URL or code.
   */
  std::string description;
  /**
   * @brief Specific value constrained field that indicates the nature of the
   * reality data.
   */
  ITwinRealityDataClassification classification;
  /**
   * @brief A key indicating the format of the data.
   *
   * The type property should be a specific indication of the format of the
   * reality data. Given a type, the consuming software should be able to
   * determine if it has the capacity to open the reality data. Although the
   * type field is a free string some specific values are reserved and other
   * values should be selected judiciously. Look at the documentation for <a
   * href="https://developer.bentley.com/apis/reality-management/rm-rd-details/#types">an
   * exhaustive list of reserved reality data types</a>.
   */
  std::string type;
  /**
   * @brief Contains the rectangular area on the Earth which encloses the
   * reality data.
   */
  CesiumGeospatial::GlobeRectangle extent;
  /**
   * @brief A boolean value that is true if the data is being created. It is
   * false if the data has been completely uploaded.
   */
  bool authoring;
};
} // namespace CesiumITwinClient