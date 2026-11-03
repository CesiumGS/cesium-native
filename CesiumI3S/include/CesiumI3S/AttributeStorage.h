#pragma once
#include <CesiumI3S/Library.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CesiumI3S {

/** @brief Primitive scalar data type used in geometry headers and attribute
 * buffers (headerAttribute.cmn.md, value.cmn.md). */
enum class DataType {
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  Float32,
  Float64,
  /** @brief Only valid in attribute value descriptors (not geometry headers).
   */
  String
};

/** @brief Header entry in a geometry resource buffer (headerAttribute.cmn.md).
 */
struct CESIUMI3S_API HeaderAttribute {
  /** @brief Name of the header property (e.g. "vertexCount", "featureCount").
   */
  std::string property;
  /** @brief Data type of the header value. */
  DataType type = DataType::UInt32;
};

/** @brief Header entry in an attribute binary buffer (headerValue.cmn.md). */
struct CESIUMI3S_API HeaderValue {
  /** @brief Semantic of the header value (headerValue.cmn.md). */
  enum class Property {
    /** @brief Number of attribute values in the buffer. */
    Count,
    /** @brief Total byte count of all attribute values. */
    AttributeValuesByteCount
  };
  /** @brief Data type of the header value. */
  DataType valueType = DataType::UInt32;
  /** @brief Semantic of the header value. */
  Property property = Property::Count;
};

/** @brief Descriptor for a section of an attribute binary buffer
 * (value.cmn.md). */
struct CESIUMI3S_API Value {
  /** @brief Encoding method for time-valued attributes (value.cmn.md). */
  enum class TimeEncoding {
    /** @brief DateTime string formatted per ECMA-ISO 8601. */
    ECMAIS8601
  };
  /** @brief Defines the value type. */
  DataType valueType = DataType::UInt32;
  /** @brief Encoding method for the value (e.g. "UTF-8"). */
  std::optional<std::string> encoding;
  /** @brief Encoding method for the time value. */
  std::optional<TimeEncoding> timeEncoding;
  /** @brief Number of values per element. */
  std::optional<uint32_t> valuesPerElement;
};

/** @brief Binary storage layout for one attribute field
 * (attributeStorageInfo.cmn.md). */
struct CESIUMI3S_API AttributeStorageInfo {
  /** @brief LEPCC-based encoding for compressed attribute binary buffers
   * (attributeInfo.pcsl.md). Only used in PCSL layers. */
  /** @brief Ordering of sections within the binary buffer
   * (attributeStorageInfo.cmn.md). */
  enum class Ordering {
    /** @brief Should only be present when working with string data types. */
    AttributeByteCounts,
    /** @brief Should always be present. */
    AttributeValues,
    ObjectIds
  };
  /** @brief LEPCC-based encoding for compressed attribute binary buffers
   * (attributeInfo.pcsl.md). Only used in PCSL layers. */
  enum class Encoding {
    /** @brief No binary buffer; use point.z from geometry. */
    EmbeddedElevation,
    /** @brief LEPCC compression for scaled integral type. */
    LepccIntensity,
    /** @brief LEPCC color compression for 3-channel RGB 8-bit. */
    LepccRgb
  };
  /** @brief The unique field identifier key. */
  std::string key;
  /** @brief The name of the field. */
  std::string name;
  /** @brief Headers of the binary attribute data. */
  std::vector<HeaderValue> header;
  /** @brief Ordering of sections in the binary buffer. */
  std::optional<std::vector<Ordering>> ordering;
  /** @brief Description for value encoding. */
  std::optional<Value> attributeValues;
  /** @brief For string types only. Byte count of each string including the null
   * character. */
  std::optional<Value> attributeByteCounts;
  /** @brief Object-id values of each feature within the node. */
  std::optional<Value> objectIds;
  /** @brief LEPCC-based encoding of the attribute binary buffer. Only used in
   * PCSL layers. */
  std::optional<Encoding> encoding;
};

/** @brief Per-feature attributes declared in the geometry
 * (featureAttribute.cmn.md). */
struct CESIUMI3S_API FeatureAttribute {
  /** @brief ID of the feature attribute. */
  std::optional<Value> id;
  /** @brief Face range of the feature attribute. */
  std::optional<Value> faceRange;
};

} // namespace CesiumI3S
