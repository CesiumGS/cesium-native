#pragma once

#include "EnumJsonHandler.h"
#include "FixedArrayJsonHandler.h"

#include <CesiumI3S/SpatialReference.h>
#include <CesiumJsonReader/BoolJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class SpatialReferenceJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::SpatialReference;
  SpatialReferenceJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::SpatialReference* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::SpatialReference* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _latestVcsWkid;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _latestWkid;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _vcsWkid;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _wkid;
  CesiumJsonReader::StringJsonHandler _wkt;
};

class HeightModelInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::HeightModelInfo;
  HeightModelInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::HeightModelInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::HeightModelInfo* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::HeightModel> _heightModel;
  CesiumJsonReader::StringJsonHandler _vertCrs;
  EnumJsonHandler<CesiumI3S::HeightUnit> _heightUnit;
};

class ObbJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::OrientedBoundingBox;
  ObbJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::OrientedBoundingBox* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::OrientedBoundingBox* _pObject = nullptr;
  FixedArrayJsonHandler<double, 3> _center;
  FixedArrayJsonHandler<double, 3> _halfSize;
  FixedArrayJsonHandler<double, 4> _quaternion;
};

class MinimumBoundingSphereJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::MinimumBoundingSphere;
  MinimumBoundingSphereJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::MinimumBoundingSphere* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::MinimumBoundingSphere* _pObject = nullptr;
  FixedArrayJsonHandler<double, 3> _center;
  CesiumJsonReader::DoubleJsonHandler _radius;
};

class ElevationInfoJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::ElevationInfo;
  ElevationInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::ElevationInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::ElevationInfo* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::ElevationMode> _mode;
  CesiumJsonReader::DoubleJsonHandler _offset;
  EnumJsonHandler<CesiumI3S::HeightUnit> _unit;
};

class FullExtentJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::FullExtent;
  FullExtentJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::FullExtent* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::FullExtent* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _xmin;
  CesiumJsonReader::DoubleJsonHandler _ymin;
  CesiumJsonReader::DoubleJsonHandler _xmax;
  CesiumJsonReader::DoubleJsonHandler _ymax;
  CesiumJsonReader::DoubleJsonHandler _zmin;
  CesiumJsonReader::DoubleJsonHandler _zmax;
  SpatialReferenceJsonHandler _spatialReference;
};

class ExtentJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Extent;
  ExtentJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Extent* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Extent* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _xmin;
  CesiumJsonReader::DoubleJsonHandler _ymin;
  CesiumJsonReader::DoubleJsonHandler _xmax;
  CesiumJsonReader::DoubleJsonHandler _ymax;
};

} // namespace CesiumI3SReader
