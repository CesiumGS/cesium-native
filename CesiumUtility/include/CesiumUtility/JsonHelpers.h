#pragma once

#include <glm/fwd.hpp>
#include <rapidjson/fwd.h>

#include <optional>
#include <string>
#include <vector>

namespace CesiumUtility {

/**
 * @brief A collection of helper functions to make reading JSON simpler.
 */
class JsonHelpers final {
public:
  /**
   * @brief Attempts to read the value at `key` of `tileJson` as a `double`,
   * returning std::nullopt if it wasn't found or couldn't be read as a double.
   *
   * @param tileJson The JSON object to obtain the scalar property from.
   * @param key The key of the scalar property to obtain.
   */
  static std::optional<double>
  getScalarProperty(const rapidjson::Value& tileJson, const std::string& key);
  /**
   * @brief Attempts to read the value at `key` of `tileJson` as a
   * `glm::dmat4x4`, returning std::nullopt if it wasn't found or couldn't be
   * read as a glm::dmat4x4.
   *
   * @param tileJson The JSON object to obtain the transform property from.
   * @param key The key of the transform property.
   */
  static std::optional<glm::dmat4x4> getTransformProperty(
      const rapidjson::Value& tileJson,
      const std::string& key);

  /**
   * @brief Obtains an array of numbers from the given JSON.
   *
   * If the property is not found, or is not an array, or does contain
   * elements that are not numbers, then `std::nullopt` is returned.
   *
   * If the given expected size is not negative, and the actual size of the
   * array does not match the expected size, then `nullopt` is returned.
   *
   * @param json The JSON object.
   * @param expectedSize The expected size of the array.
   * @param key The key (property name) of the array.
   * @return The array, or `nullopt`.
   */
  static std::optional<std::vector<double>> getDoubles(
      const rapidjson::Value& json,
      int32_t expectedSize,
      const std::string& key);

  /**
   * @brief Attempts to obtain a string from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the string.
   * @param defaultValue The default value to return if the string property
   * `key` of `json` couldn't be read.
   */
  static std::string getStringOrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      const std::string& defaultValue);
  /**
   * @brief Attempts to read `json` as a string, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a string.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a string.
   */
  static std::string getStringOrDefault(
      const rapidjson::Value& json,
      const std::string& defaultValue);

  /**
   * @brief Attempts to obtain a double from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the double.
   * @param defaultValue The default value to return if the double property
   * `key` of `json` couldn't be read.
   */
  static double getDoubleOrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      double defaultValue);
  /**
   * @brief Attempts to read `json` as a double, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a double.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a double.
   */
  static double
  getDoubleOrDefault(const rapidjson::Value& json, double defaultValue);

  /**
   * @brief Attempts to obtain a uint32_t from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the uint32_t.
   * @param defaultValue The default value to return if the uint32_t property
   * `key` of `json` couldn't be read.
   */
  static uint32_t getUint32OrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      uint32_t defaultValue);
  /**
   * @brief Attempts to read `json` as a uint32_t, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a uint32_t.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a uint32_t.
   */
  static uint32_t
  getUint32OrDefault(const rapidjson::Value& json, uint32_t defaultValue);

  /**
   * @brief Attempts to obtain a int32_t from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the int32_t.
   * @param defaultValue The default value to return if the int32_t property
   * `key` of `json` couldn't be read.
   */
  static int32_t getInt32OrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      int32_t defaultValue);
  /**
   * @brief Attempts to read `json` as a int32_t, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a int32_t.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a int32_t.
   */
  static int32_t
  getInt32OrDefault(const rapidjson::Value& json, int32_t defaultValue);

  /**
   * @brief Attempts to obtain a uint64_t from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the uint64_t.
   * @param defaultValue The default value to return if the uint64_t property
   * `key` of `json` couldn't be read.
   */
  static uint64_t getUint64OrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      uint64_t defaultValue);
  /**
   * @brief Attempts to read `json` as a uint64_t, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a uint64_t.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a uint64_t.
   */
  static uint64_t
  getUint64OrDefault(const rapidjson::Value& json, uint64_t defaultValue);

  /**
   * @brief Attempts to obtain a int64_t from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the int64_t.
   * @param defaultValue The default value to return if the int64_t property
   * `key` of `json` couldn't be read.
   */
  static int64_t getInt64OrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      int64_t defaultValue);
  /**
   * @brief Attempts to read `json` as a int64_t, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a int64_t.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a int64_t.
   */
  static int64_t
  getInt64OrDefault(const rapidjson::Value& json, int64_t defaultValue);

  /**
   * @brief Attempts to obtain a bool from the given key on the JSON object,
   * returning a default value if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the bool.
   * @param defaultValue The default value to return if the bool property
   * `key` of `json` couldn't be read.
   */
  static bool getBoolOrDefault(
      const rapidjson::Value& json,
      const std::string& key,
      bool defaultValue);
  /**
   * @brief Attempts to read `json` as a bool, returning a default value if
   * this isn't possible.
   *
   * @param json The JSON value that might be a bool.
   * @param defaultValue The default value to return if `json` couldn't be read
   * as a bool.
   */
  static bool getBoolOrDefault(const rapidjson::Value& json, bool defaultValue);

  /**
   * @brief Attempts to read an array of strings from the property `key` of
   * `json`, returning an empty vector if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the string array.
   */
  static std::vector<std::string>
  getStrings(const rapidjson::Value& json, const std::string& key);

  /**
   * @brief Attempts to read an int64_t array from the property `key` of
   * `json`, returning an empty vector if this isn't possible.
   *
   * @param json The JSON object.
   * @param key The key (property name) of the int64_t array.
   */
  static std::vector<int64_t>
  getInt64s(const rapidjson::Value& json, const std::string& key);
};

} // namespace CesiumUtility
