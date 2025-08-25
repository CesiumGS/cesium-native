#pragma once

#include <span>
#include <string>

namespace CesiumClientCommon {
/**
 * @brief Attempts to parse a JSON error response from the provided buffer.
 *
 * Two schemas of JSON error document are supported.
 *
 * ### Simple
 * ```json
 * {
 *     "error": "error_code",
 *     "error_description": "A longer user-friendly error message."
 * }
 * ```
 *
 * `error` becomes `outError` and `error_description` becomes `outErrorDesc`.
 *
 * ### Detailed
 * ```json
 * {
 *     "error": {
 *         "code": "error_code",
 *         "message": "A longer user-friendly error message.",
 *         "details": [
 *             {
 *                 "code": "error_code",
 *                 "message": "A longer user-friendly error message.",
 *                 "target": "field_name"
 *             }
 *         ]
 *     }
 * }
 * ```
 *
 * `error.code` becomes `outError`. `error.message` and any entries in
 * `error.details` are combined together into `outErrorDesc`.
 *
 * @param body A response body that might contain JSON object with error
 * information.
 * @param outError A string that will be set to the error code, if one is found.
 * @param outErrorDesc A string that will be set to a more detailed error
 * message, if one is found.
 *
 * @returns True if a JSON error message was found in the provided body, false
 * otherwise.
 */
bool parseErrorResponse(
    const std::span<const std::byte>& body,
    std::string& outError,
    std::string& outErrorDesc);
} // namespace CesiumClientCommon
