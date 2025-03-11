#pragma once

#include <span>
#include <string>

namespace CesiumClientCommon {
bool parseErrorResponse(
    const std::span<const std::byte>& body,
    std::string& outError,
    std::string& outErrorDesc);
}