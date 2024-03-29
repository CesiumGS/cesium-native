#pragma once

#include "HttpHeaders.h"
#include "Library.h"

#include <gsl/span>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace CesiumAsync {

/**
 * @brief A completed response for a 3D Tiles asset.
 */
class CESIUMASYNC_API IAssetResponse {
public:
  /**
   * @brief Default destructor
   */
  virtual ~IAssetResponse() = default;

  /**
   * @brief Returns the HTTP response code.
   */
  virtual uint16_t statusCode() const = 0;

  /**
   * @brief Returns the HTTP content type
   */
  virtual std::string contentType() const = 0;

  /**
   * @brief Returns the HTTP headers of the response
   */
  virtual const HttpHeaders& headers() const = 0;

  /**
   * @brief Returns the data of this response
   */
  virtual gsl::span<const std::byte> data() const = 0;
};

} // namespace CesiumAsync
