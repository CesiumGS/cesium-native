// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <CesiumGltf/ExtensionModelMaxarMeshVariantsValue.h>
#include <CesiumGltfReader/Library.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonReader/JsonReaderOptions.h>

#include <rapidjson/fwd.h>

#include <span>
#include <vector>

namespace CesiumGltf {
struct ExtensionModelMaxarMeshVariantsValue;
} // namespace CesiumGltf

namespace CesiumGltfReader {

/**
 * @brief Reads \ref CesiumGltf::ExtensionModelMaxarMeshVariantsValue
 * "ExtensionModelMaxarMeshVariantsValue" instances from JSON.
 */
class CESIUMGLTFREADER_API ExtensionModelMaxarMeshVariantsValueReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  ExtensionModelMaxarMeshVariantsValueReader();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  CesiumJsonReader::JsonReaderOptions& getOptions();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  const CesiumJsonReader::JsonReaderOptions& getOptions() const;

  /**
   * @brief Reads an instance of ExtensionModelMaxarMeshVariantsValue from a
   * byte buffer.
   *
   * @param data The buffer from which to read the instance.
   * @return The result of reading the instance.
   */
  CesiumJsonReader::ReadJsonResult<
      CesiumGltf::ExtensionModelMaxarMeshVariantsValue>
  readFromJson(const std::span<const std::byte>& data) const;

  /**
   * @brief Reads an instance of ExtensionModelMaxarMeshVariantsValue from a
   * rapidJson::Value.
   *
   * @param value The value from which to read the instance.
   * @return The result of reading the instance.
   */
  CesiumJsonReader::ReadJsonResult<
      CesiumGltf::ExtensionModelMaxarMeshVariantsValue>
  readFromJson(const rapidjson::Value& value) const;

  /**
   * @brief Reads an array of instances of ExtensionModelMaxarMeshVariantsValue
   * from a rapidJson::Value.
   *
   * @param value The value from which to read the array of instances.
   * @return The result of reading the array of instances.
   */
  CesiumJsonReader::ReadJsonResult<
      std::vector<CesiumGltf::ExtensionModelMaxarMeshVariantsValue>>
  readArrayFromJson(const rapidjson::Value& value) const;

private:
  CesiumJsonReader::JsonReaderOptions _options;
};

} // namespace CesiumGltfReader
