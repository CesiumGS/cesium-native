#pragma once

namespace CesiumAsync {

enum class PriorityGroup {
  /**
   * @brief Low priority work that isn't needed right now, but is being
   * prepared for the future.
   */
  Preload = 0,

  /**
   * @brief Normal priority work that is needed as soon as practical.
   */
  Normal = 1,

  /**
   * @brief High priority work that is blocking other work.
   */
  Urgent = 2
};

}
