#pragma once

#include "AttributeStorageJsonHandlers.h"
#include "FixedArrayJsonHandler.h"
#include "GeometryJsonHandlers.h"

#include <CesiumI3S/FeatureData.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class FeatureDataJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::FeatureData;
  FeatureDataJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::FeatureData* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::FeatureData* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<uint64_t> _id;
  CesiumJsonReader::
      ArrayJsonHandler<double, CesiumJsonReader::DoubleJsonHandler>
          _position;
  FixedArrayJsonHandler<double, 3> _pivotOffset;
  FixedArrayJsonHandler<double, 6> _mbb;
  CesiumJsonReader::StringJsonHandler _layer;
  FeatureAttributeJsonHandler _attributes;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Geometry, GeometryJsonHandler>
      _geometries;
};

} // namespace CesiumI3SReader
