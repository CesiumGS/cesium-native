#pragma once

#include "Library.h"

#include <Cesium3DTiles/GroupMetadata.h>
#include <Cesium3DTiles/Schema.h>

#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

struct CESIUM3DTILESSELECTION_API TilesetMetadata {
  /**
   * @brief An object defining the structure of metadata classes and enums. When
   * this is defined, then `schemaUri` shall be undefined.
   */
  std::optional<Cesium3DTiles::Schema> schema;

  /**
   * @brief The URI (or IRI) of the external schema file. When this is defined,
   * then `schema` shall be undefined.
   */
  std::optional<std::string> schemaUri;

  /**
   * @brief An array of groups that tile content may belong to. Each element of
   * this array is a metadata entity that describes the group. The tile content
   * `group` property is an index into this array.
   */
  std::vector<Cesium3DTiles::GroupMetadata> groups;

  /**
   * @brief A metadata entity that is associated with this tileset.
   */
  std::optional<Cesium3DTiles::MetadataEntity> metadata;
};

} // namespace Cesium3DTilesSelection
