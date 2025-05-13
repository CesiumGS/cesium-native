#pragma once

#include <cstdint>
#include <string>

namespace CesiumITwinClient {
/**
 * @brief The status of the iTwin.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwinstatus
 * for more information.
 */
enum class ITwinStatus : uint8_t {
  Unknown = 0,
  Active = 1,
  Inactive = 2,
  Trial = 3
};

/**
 * @brief Obtains an \ref ITwinStatus value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwinstatus
 * for a list of possible values.
 */
ITwinStatus iTwinStatusFromString(const std::string& str);

/**
 * @brief Information on a single iTwin.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwin-summary
 * for more information.
 */
struct ITwin {
  /** @brief The iTwin Id. */
  std::string id;
  /**
   * @brief The `Class` of your iTwin.
   *
   * See
   * https://developer.bentley.com/apis/itwins/overview/#itwin-classes-and-subclasses
   * for more information.
   */
  std::string iTwinClass;
  /**
   * @brief The `subClass` of your iTwin.
   *
   * See
   * https://developer.bentley.com/apis/itwins/overview/#itwin-classes-and-subclasses
   * for more information.
   */
  std::string subClass;
  /** @brief An open ended property to better define your iTwin's Type. */
  std::string type;
  /**
   * @brief A unique number or code for the iTwin.
   *
   * This is the value that uniquely identifies the iTwin within your
   * organization.
   */
  std::string number;
  /** @brief A display name for the iTwin. */
  std::string displayName;
  /** @brief The status of the iTwin. */
  ITwinStatus status;
};
} // namespace CesiumITwinClient