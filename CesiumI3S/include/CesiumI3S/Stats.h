#pragma once
#include <CesiumI3S/Library.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace CesiumI3S {

/** @brief A value and its occurrence count (statisticsInfo.cmn.md). */
struct CESIUMI3S_API ValueCount {
  /** @brief The attribute value. */
  std::variant<double, std::string> value;
  /** @brief Number of times this value appears. */
  uint64_t count = 0;
};

/** @brief Histogram distribution of attribute values (statisticsInfo.cmn.md).
 */
struct CESIUMI3S_API Histogram {
  /** @brief Minimum value of the range. */
  double minimum = 0.0;
  /** @brief Maximum value of the range. */
  double maximum = 0.0;
  /** @brief Number of values in each bin. */
  std::vector<uint64_t> counts;
};

/** @brief Statistical summary for a single attribute field
 * (statisticsInfo.cmn.md). */
struct CESIUMI3S_API StatsInfo {
  /** @brief Total number of values. */
  std::optional<uint64_t> totalValuesCount;
  /** @brief Minimum value. */
  std::optional<double> min;
  /** @brief Maximum value. */
  std::optional<double> max;
  /** @brief Minimum value as a time string. */
  std::optional<std::string> minTimeStr;
  /** @brief Maximum value as a time string. */
  std::optional<std::string> maxTimeStr;
  /** @brief Number of non-null values. */
  std::optional<uint64_t> count;
  /** @brief Sum of all values. */
  std::optional<double> sum;
  /** @brief Mean of all values. */
  std::optional<double> avg;
  /** @brief Standard deviation. */
  std::optional<double> stddev;
  /** @brief Variance. */
  std::optional<double> variance;
  /** @brief Value distribution histogram. */
  std::optional<Histogram> histogram;
  /** @brief Most frequently occurring values. */
  std::optional<std::vector<ValueCount>> mostFrequentValues;
};

/** @brief Statistics document for a single attribute field
 * (statisticsInfo.cmn.md). */
struct CESIUMI3S_API Statistics {
  /** @brief Statistical summary for the field. */
  std::optional<StatsInfo> stats;
};

/** @brief Reference to the statistics resource for a field
 * (statisticsInfo.cmn.md). */
struct CESIUMI3S_API StatisticsInfo {
  /** @brief Attribute key (e.g. "f_0"). */
  std::string key;
  /** @brief Attribute name. */
  std::string name;
  /** @brief Relative URL to the statistics document. */
  std::string href;
};

} // namespace CesiumI3S
