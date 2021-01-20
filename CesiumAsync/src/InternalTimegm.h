#pragma once

#include <ctime>

namespace CesiumAsync {
    time_t internalTimegm(std::tm const* t);
}
