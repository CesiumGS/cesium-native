// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <Cesium3DTiles/EnumValue.h>
#include <Cesium3DTiles/Library.h>
#include <CesiumUtility/ExtensibleObject.h>

#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTiles {
/**
 * @brief An object defining the values of an enum.
 */
struct CESIUM3DTILES_API Enum final : public CesiumUtility::ExtensibleObject {
  /**
   * @brief The original name of this type.
   */
  static constexpr const char* TypeName = "Enum";

  /**
   * @brief Known values for The type of the integer enum value.
   */
  struct ValueType {
    /** @brief `INT8` */
    inline static const std::string INT8 = "INT8";

    /** @brief `UINT8` */
    inline static const std::string UINT8 = "UINT8";

    /** @brief `INT16` */
    inline static const std::string INT16 = "INT16";

    /** @brief `UINT16` */
    inline static const std::string UINT16 = "UINT16";

    /** @brief `INT32` */
    inline static const std::string INT32 = "INT32";

    /** @brief `UINT32` */
    inline static const std::string UINT32 = "UINT32";

    /** @brief `INT64` */
    inline static const std::string INT64 = "INT64";

    /** @brief `UINT64` */
    inline static const std::string UINT64 = "UINT64";
  };

  /**
   * @brief The name of the enum, e.g. for display purposes.
   */
  std::optional<std::string> name;

  /**
   * @brief The description of the enum.
   */
  std::optional<std::string> description;

  /**
   * @brief The type of the integer enum value.
   *
   * Known values are defined in {@link ValueType}.
   *
   */
  std::string valueType = ValueType::UINT16;

  /**
   * @brief An array of enum values. Duplicate names or duplicate integer values
   * are not allowed.
   */
  std::vector<Cesium3DTiles::EnumValue> values;

  /**
   * @brief Calculates the size in bytes of this object, including the contents
   * of all collections, pointers, and strings. This will NOT include the size
   * of any extensions attached to the object. Calling this method may be slow
   * as it requires traversing the object's entire structure.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += int64_t(sizeof(Enum));
    accum += CesiumUtility::ExtensibleObject::getSizeBytes() -
             int64_t(sizeof(CesiumUtility::ExtensibleObject));
    if (this->name) {
      accum += int64_t(this->name->capacity() * sizeof(char));
    }
    if (this->description) {
      accum += int64_t(this->description->capacity() * sizeof(char));
    }
    accum +=
        int64_t(sizeof(Cesium3DTiles::EnumValue) * this->values.capacity());
    for (const Cesium3DTiles::EnumValue& value : this->values) {
      accum += value.getSizeBytes() - int64_t(sizeof(Cesium3DTiles::EnumValue));
    }
    return accum;
  }
};
} // namespace Cesium3DTiles
