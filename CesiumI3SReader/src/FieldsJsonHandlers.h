#pragma once

#include "EnumJsonHandler.h"
#include "FixedArrayJsonHandler.h"
#include "VariantDoubleStringJsonHandler.h"

#include <CesiumI3S/Fields.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class DomainCodedValueJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::DomainCodedValue;
  DomainCodedValueJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::DomainCodedValue* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::DomainCodedValue* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _name;
  VariantDoubleStringJsonHandler _code;
};

class DomainJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Domain;
  DomainJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Domain* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Domain* _pObject = nullptr;
  EnumJsonHandler<CesiumI3S::Domain::Type> _type;
  CesiumJsonReader::StringJsonHandler _name;
  CesiumJsonReader::StringJsonHandler _description;
  EnumJsonHandler<CesiumI3S::Domain::FieldType> _fieldType;
  FixedArrayJsonHandler<double, 2> _range;
  CesiumJsonReader::
      ArrayJsonHandler<CesiumI3S::DomainCodedValue, DomainCodedValueJsonHandler>
          _codedValues;
  EnumJsonHandler<CesiumI3S::Domain::MergePolicy> _mergePolicy;
  EnumJsonHandler<CesiumI3S::Domain::SplitPolicy> _splitPolicy;
};

class FieldJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Field;
  FieldJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Field* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Field* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _name;
  EnumJsonHandler<CesiumI3S::Field::Type> _type;
  CesiumJsonReader::StringJsonHandler _alias;
  DomainJsonHandler _domain;
};

} // namespace CesiumI3SReader
