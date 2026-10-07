#pragma once

#include "EnumJsonHandler.h"

#include <CesiumI3S/AttributeStorage.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class HeaderAttributeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::HeaderAttribute;
  HeaderAttributeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::HeaderAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::HeaderAttribute* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _property;
  EnumJsonHandler<CesiumI3S::DataType> _type;
};

class HeaderValueJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::HeaderValue;
  HeaderValueJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::HeaderValue* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::HeaderValue* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::DataType> _valueType;
  EnumJsonHandler<CesiumI3S::HeaderValue::Property> _property;
};

class ValueJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Value;
  ValueJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Value* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Value* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::DataType> _valueType;
  CesiumJsonReader::StringJsonHandler _encoding;
  EnumJsonHandler<CesiumI3S::Value::TimeEncoding> _timeEncoding;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _valuesPerElement;
};

class AttributeStorageInfoJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::AttributeStorageInfo;
  AttributeStorageInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::AttributeStorageInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::AttributeStorageInfo* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _key;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::HeaderValue, HeaderValueJsonHandler>
          _header;
  EnumArrayJsonHandler<CesiumI3S::AttributeStorageInfo::Ordering> _ordering;
  ValueJsonHandler _attributeValues;
  ValueJsonHandler _attributeByteCounts;
  ValueJsonHandler _objectIds;
  EnumJsonHandler<CesiumI3S::AttributeStorageInfo::Encoding> _encoding;
};

class FeatureAttributeJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::FeatureAttribute;
  FeatureAttributeJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::FeatureAttribute* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::FeatureAttribute* _pObject = nullptr;
  ValueJsonHandler _id;
  ValueJsonHandler _faceRange;
};

} // namespace CesiumI3SReader
