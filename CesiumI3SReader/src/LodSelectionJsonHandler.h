#pragma once

#include "EnumJsonHandler.h"

#include <CesiumI3S/LodSelection.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>

namespace CesiumI3SReader {

class LodSelectionJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::LodSelection;
  LodSelectionJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::LodSelection* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::LodSelection* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::LodSelection::Metric> _metricType;
  CesiumJsonReader::DoubleJsonHandler _maxError;
};

} // namespace CesiumI3SReader
