#pragma once

#include "Library.h"

#include <CesiumUtility/joinToString.h>

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
/**
 * @brief The container to store the error and warning list when loading a tile
 * or glTF content
 */
struct CESIUM3DTILESSELECTION_API ErrorList {
  /**
   * @brief Merge the errors and warnings from other ErrorList together
   *
   * @param errorList The other instance of ErrorList that will be merged with
   * the current instance.
   */
  void merge(const ErrorList& errorList);

  /**
   * @brief Merge the errors and warnings from other ErrorList together
   *
   * @param errorList The other instance of ErrorList that will be merged with
   * the current instance.
   */
  void merge(ErrorList&& errorList);

  /**
   * @brief Add an error message
   *
   * @param error The error message to be added.
   */
  template <typename ErrorStr> void emplaceError(ErrorStr&& error) {
    errors.emplace_back(std::forward<ErrorStr>(error));
  }

  /**
   * @brief Add a warning message
   *
   * @param error The warning message to be added.
   */
  template <typename WarningStr> void emplaceWarning(WarningStr&& warning) {
    warnings.emplace_back(std::forward<WarningStr>(warning));
  }

  /**
   * @brief Check if there are any error messages.
   */
  bool hasErrors() const noexcept;

  /**
   * @brief Log all the error messages.
   *
   * @param pLogger The logger to log the messages
   * @param prompt The message prompt for the error messages.
   */
  template <typename PromptStr>
  void logError(
      const std::shared_ptr<spdlog::logger>& pLogger,
      PromptStr&& prompt) const noexcept {
    if (!errors.empty()) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "{}:\n- {}",
          std::forward<PromptStr>(prompt),
          CesiumUtility::joinToString(errors, "\n- "));
    }
  }

  /**
   * @brief Log all the warning messages
   *
   * @param pLogger The logger to log the messages
   * @param prompt The message prompt for the warning messages.
   */
  template <typename PromptStr>
  void logWarning(
      const std::shared_ptr<spdlog::logger>& pLogger,
      PromptStr&& prompt) const noexcept {
    if (!warnings.empty()) {
      SPDLOG_LOGGER_WARN(
          pLogger,
          "{}:\n- {}",
          std::forward<PromptStr>(prompt),
          CesiumUtility::joinToString(warnings, "\n- "));
    }
  }

  /**
   * @brief Check if there are any error messages.
   */
  explicit operator bool() const noexcept;

  /**
   * @brief The error messages of this container
   */
  std::vector<std::string> errors;

  /**
   * @brief The warning messages of this container
   */
  std::vector<std::string> warnings;
};
} // namespace Cesium3DTilesSelection
