#pragma once

#include <CesiumUtility/Library.h>
#include <CesiumUtility/Log.h> // NOLINT
#include <CesiumUtility/joinToString.h>

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace CesiumUtility {

/**
 * @brief The container to store the error and warning list when loading a tile
 * or glTF content
 */
struct CESIUMUTILITY_API ErrorList {
  /**
   * @brief Creates an {@link ErrorList} containing a single error.
   *
   * @param errorMessage The error message.
   * @return The new list containing the single error.
   */
  static ErrorList error(std::string errorMessage);

  /**
   * @brief Creates an {@link ErrorList} containing a single warning.
   *
   * @param warningMessage The warning message.
   * @return The new list containing the single warning.
   */
  static ErrorList warning(std::string warningMessage);

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
   * @param warning The warning message to be added.
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
   * @brief Log all the error and warning messages
   *
   * @param pLogger The logger to log the messages
   * @param prompt The message prompt for the messages.
   */
  template <typename PromptStr>
  void
  log(const std::shared_ptr<spdlog::logger>& pLogger,
      PromptStr&& prompt) const noexcept {
    if (!this->errors.empty()) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          this->format(std::forward<PromptStr>(prompt)));
    } else if (!this->warnings.empty()) {
      SPDLOG_LOGGER_WARN(
          pLogger,
          this->format(std::forward<PromptStr>(prompt)));
    }
  }

  /**
   * @brief Format all of the errors and warnings into a single string.
   *
   * @param prompt The message prompt for the messages.
   * @returns The formatted message, or an empty string if there are no errors
   * or warnings.
   */
  template <typename PromptStr>
  std::string format(PromptStr&& prompt) const noexcept {
    if (this->warnings.empty() && this->errors.empty()) {
      return std::string();
    }

    std::string result = prompt;

    if (!this->errors.empty()) {
      result += "\n- [Error] " +
                CesiumUtility::joinToString(this->errors, "\n- [Error] ");
    }

    if (!this->warnings.empty()) {
      result += "\n- [Warning] " +
                CesiumUtility::joinToString(this->warnings, "\n- [Warning] ");
    }

    return result;
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

} // namespace CesiumUtility
