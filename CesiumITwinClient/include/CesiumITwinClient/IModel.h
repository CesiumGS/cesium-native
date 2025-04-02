#pragma once

#include <CesiumGeospatial/GlobeRectangle.h>

#include <cstdint>
#include <string>

namespace CesiumITwinClient {
/**
 * @brief The possible states for an iModel.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel-state
 * for more information.
 */
enum class IModelState : uint8_t {
  Unknown = 0,
  Initialized = 1,
  NotInitialized = 2
};

/**
 * @brief Obtains an \ref IModelState value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel-state
 * for a list of possible values.
 */
IModelState iModelStateFromString(const std::string& str);

/**
 * @brief An iModel.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel
 * for more information.
 */
struct IModel {
  /** @brief Id of the iModel. */
  std::string id;
  /** @brief Display name of the iModel. */
  std::string displayName;
  /** @brief Name of the iModel. */
  std::string name;
  /** @brief Description of the iModel. */
  std::string description;
  /** @brief Indicates the state of the iModel. */
  IModelState state;
  /**
   * @brief The maximum rectangular area on the Earth which encloses the iModel.
   */
  CesiumGeospatial::GlobeRectangle extent;
};
}; // namespace CesiumITwinClient