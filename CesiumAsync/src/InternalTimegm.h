#pragma once

#include <ctime>

namespace CesiumAsync {
// The boost chrono's implementation of linux timegm
// (https://man7.org/linux/man-pages/man3/timegm.3.html) The original boost
// implementation can be found at this link
// https://github.com/boostorg/chrono/blob/aa51cbd5121ed29093484f53e5f96e13a9a915b4/include/boost/chrono/io/time_point_io.hpp#L784
time_t internalTimegm(std::tm const* t);
} // namespace CesiumAsync
