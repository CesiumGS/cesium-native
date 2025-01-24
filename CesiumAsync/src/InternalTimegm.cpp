// All the code below are from the boost chrono's implementation of linux timegm
// (https://man7.org/linux/man-pages/man3/timegm.3.html) The original boost
// implementation can be found at this link
// https://github.com/boostorg/chrono/blob/aa51cbd5121ed29093484f53e5f96e13a9a915b4/include/boost/chrono/io/time_point_io.hpp#L784

#include "InternalTimegm.h"

#include <cstdint>
#include <ctime>

namespace CesiumAsync {

namespace {
int32_t isLeap(int32_t year) {
  if (year % 400 == 0)
    return 1;
  if (year % 100 == 0)
    return 0;
  if (year % 4 == 0)
    return 1;
  return 0;
}

int32_t daysFrom0(int32_t year) {
  year--;
  return 365 * year + (year / 400) - (year / 100) + (year / 4);
}

int32_t daysFrom1970(int32_t year) {
  static const int days_from_0_to_1970 = daysFrom0(1970);
  return daysFrom0(year) - days_from_0_to_1970;
}

int32_t daysFrom1jan(int32_t year, int32_t month, int32_t day) {
  static const int32_t days[2][12] = {
      {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
      {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};

  return days[isLeap(year)][month - 1] + day - 1;
}
} // namespace

time_t internalTimegm(std::tm const* t) {
  int year = t->tm_year + 1900;
  int month = t->tm_mon;
  if (month > 11) {
    year += month / 12;
    month %= 12;
  } else if (month < 0) {
    const int years_diff = (-month + 11) / 12;
    year -= years_diff;
    month += 12 * years_diff;
  }
  month++;
  const int day = t->tm_mday;
  const int day_of_year = daysFrom1jan(year, month, day);
  const time_t days_since_epoch =
      static_cast<time_t>(daysFrom1970(year) + day_of_year);

  const time_t seconds_in_day = static_cast<time_t>(3600 * 24);
  const time_t result =
      seconds_in_day * days_since_epoch +
      static_cast<time_t>(3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec);

  return result;
}

} // namespace CesiumAsync
