#pragma once

#include "VariantDoubleStringJsonHandler.h"

#include <CesiumI3S/Stats.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class ValueCountJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::ValueCount;
  ValueCountJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::ValueCount* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::ValueCount* _pObject = nullptr;
  VariantDoubleStringJsonHandler _value;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _count;
};

class HistogramJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Histogram;
  HistogramJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Histogram* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Histogram* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _minimum;
  CesiumJsonReader::DoubleJsonHandler _maximum;
  CesiumJsonReader::
      ArrayJsonHandler<uint64_t, CesiumJsonReader::IntegerJsonHandler<uint64_t>>
          _counts;
};

class StatsInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::StatsInfo;
  StatsInfoJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::StatsInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::StatsInfo* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _totalValuesCount;
  CesiumJsonReader::DoubleJsonHandler _min;
  CesiumJsonReader::DoubleJsonHandler _max;
  CesiumJsonReader::StringJsonHandler _minTimeStr;
  CesiumJsonReader::StringJsonHandler _maxTimeStr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _count;
  CesiumJsonReader::DoubleJsonHandler _sum;
  CesiumJsonReader::DoubleJsonHandler _avg;
  CesiumJsonReader::DoubleJsonHandler _stddev;
  CesiumJsonReader::DoubleJsonHandler _variance;
  HistogramJsonHandler _histogram;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::ValueCount, ValueCountJsonHandler>
          _mostFrequentValues;
};

class StatisticsJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Statistics;
  StatisticsJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::Statistics* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Statistics* _pObject = nullptr;
  StatsInfoJsonHandler _stats;
};

class StatisticsInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::StatisticsInfo;
  StatisticsInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::StatisticsInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::StatisticsInfo* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _key;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _href;
};

} // namespace CesiumI3SReader
