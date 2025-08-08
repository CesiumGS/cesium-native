#pragma once

#include <string>

namespace CesiumITwinClient {

/**
 * @brief Information on a user.
 */
struct UserProfile {
  /**
   * @brief A GUID representing this user.
   */
  std::string id;
  /**
   * @brief A display name for this user, often the same as `email`.
   */
  std::string displayName;
  /**
   * @brief The user's first name.
   */
  std::string givenName;
  /**
   * @brief The user's last name.
   */
  std::string surname;
  /**
   * @brief The user's email.
   */
  std::string email;
};

} // namespace CesiumITwinClient