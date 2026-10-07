#pragma once
#include <CesiumI3S/Library.h>

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace CesiumI3S {

/** @brief A coded value entry within a domain (domainCodedValue.cmn.md). */
struct CESIUMI3S_API DomainCodedValue {
  /** @brief Text label for this coded value. */
  std::string name;
  /** @brief The code value (numeric or string). */
  std::variant<double, std::string> code;
};

/** @brief Attribute domain — rules constraining the legal values of a field
 * (domain.cmn.md). */
struct CESIUMI3S_API Domain {
  /** @brief Domain type. */
  enum class Type { CodedValue, Range };
  /** @brief Field type a domain can be associated with. */
  enum class FieldType { Date, Single, Double, Integer, SmallInteger, String };
  /** @brief Merge policy (unused by scene layers). */
  enum class MergePolicy { MPTDefaultValue, MPTSumValues, MPTAreaWeighted };
  /** @brief Split policy (unused by scene layers). */
  enum class SplitPolicy { SPTGeometryRatio, SPTDuplicate, SPTDefaultValue };

  /** @brief Type of domain. */
  Type type = Type::CodedValue;
  /** @brief Name of the domain. Must be unique per scene layer. */
  std::string name;
  /** @brief Description of the domain. */
  std::optional<std::string> description;
  /** @brief The field type with which the domain can be associated. */
  std::optional<FieldType> fieldType;
  /** @brief [min, max] range for range-type domains. */
  std::optional<std::array<double, 2>> range;
  /** @brief Coded values for coded-value domains. */
  std::optional<std::vector<DomainCodedValue>> codedValues;
  /** @brief Merge policy. Not used by scene layers. */
  std::optional<MergePolicy> mergePolicy;
  /** @brief Split policy. Not used by scene layers. */
  std::optional<SplitPolicy> splitPolicy;
};

/** @brief Attribute field descriptor (field.cmn.md). */
struct CESIUMI3S_API Field {
  /** @brief Field value type. */
  enum class Type {
    Date,
    Single,
    Double,
    GUID,
    GlobalID,
    Integer,
    OID,
    SmallInteger,
    String
  };

  /** @brief Name of the field. */
  std::string name;
  /** @brief Type of the field. */
  Type type = Type::String;
  /** @brief Alias of the field. */
  std::optional<std::string> alias;
  /** @brief Domain defined for the field. */
  std::optional<Domain> domain;
};

} // namespace CesiumI3S
