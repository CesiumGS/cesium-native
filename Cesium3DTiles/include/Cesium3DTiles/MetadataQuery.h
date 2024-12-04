#pragma once

#include <Cesium3DTiles/MetadataEntity.h>
#include <Cesium3DTiles/Schema.h>
#include <CesiumUtility/JsonValue.h>

#include <string>
#include <unordered_map>

namespace Cesium3DTiles {

/**
 * @brief Holds the details of a found property in a {@link MetadataEntity}.
 *
 * Because this structure holds _references_ to the original {@link Schema} and
 * {@link MetadataEntity} instances, it will be invalided if either are
 * destroyed or modified. Continuing to access this result in that scenario will
 * result in undefined behavior.
 */
struct CESIUM3DTILES_API FoundMetadataProperty {
  /**
   * @brief A reference to the identifier of the class that contains the found
   * property within the {@link Schema}.
   */
  const std::string& classIdentifier;

  /**
   * @brief A reference to the {@link Class} that contains the found property
   * within the {@link Schema}.
   */
  const Class& classDefinition;

  /**
   * @brief A reference to the identifier of the found property within the
   * {@link Schema}.
   */
  const std::string& propertyIdentifier;

  /**
   * @brief A reference to the {@link ClassProperty} describing the found
   * property within the {@link Schema}.
   */
  const ClassProperty& propertyDefinition;

  /**
   * @brief A reference to the value of the found property within the
   * {@link MetadataEntity}.
   */
  const CesiumUtility::JsonValue& propertyValue;
};

/**
 * @brief Convenience functions for querying {@link MetadataEntity} instances.
 */
class CESIUM3DTILES_API MetadataQuery {
public:
  /**
   * @brief Gets the first property with a given
   * {@link ClassProperty::semantic}.
   *
   * @param schema The schema to use to look up semantics.
   * @param entity The metadata entity to search for a property with the
   * semantic.
   * @param semantic The semantic to find.
   * @return The details of the found property, or `std::nullopt` if a property
   * with the given semantic does not exist.
   */
  static std::optional<FoundMetadataProperty> findFirstPropertyWithSemantic(
      const Schema& schema,
      const MetadataEntity& entity,
      const std::string& semantic);
};

} // namespace Cesium3DTiles
