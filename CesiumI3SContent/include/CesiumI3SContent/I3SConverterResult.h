#pragma once

#include <CesiumGltf/Model.h>
#include <CesiumI3SContent/Library.h>
#include <CesiumUtility/ErrorList.h>

#include <optional>

namespace CesiumI3SContent {

/**
 * @brief The result of converting an I3S node geometry binary to a glTF model.
 */
struct CESIUMI3SCONTENT_API I3SConverterResult {
  /**
   * @brief The glTF model converted from the I3S geometry binary. Empty if
   * there were errors during conversion.
   */
  std::optional<CesiumGltf::Model> model;

  /**
   * @brief Errors and warnings produced during conversion.
   */
  CesiumUtility::ErrorList errors;
};

} // namespace CesiumI3SContent
