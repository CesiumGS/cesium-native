#include <Cesium3DTiles/Content.h>
#include <Cesium3DTiles/Tile.h>
#include <Cesium3DTiles/Tileset.h>

#include <glm/ext/matrix_double4x4.hpp>

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTiles {

namespace {

std::optional<glm::dmat4> getTileTransform(const Cesium3DTiles::Tile& tile) {
  const std::vector<double>& transform = tile.transform;

  if (transform.empty()) {
    return glm::dmat4(1.0);
  }

  if (transform.size() < 16) {
    return std::nullopt;
  }

  return glm::dmat4(
      glm::dvec4(transform[0], transform[1], transform[2], transform[3]),
      glm::dvec4(transform[4], transform[5], transform[6], transform[7]),
      glm::dvec4(transform[8], transform[9], transform[10], transform[11]),
      glm::dvec4(transform[12], transform[13], transform[14], transform[15]));
}

template <typename TCallback>
void forEachContentRecursive(
    const glm::dmat4& transform,
    const Tileset& tileset,
    const Tile& tile,
    TCallback& callback) {
  glm::dmat4 tileTransform =
      transform * getTileTransform(tile).value_or(glm::dmat4(1.0));

  if (tile.content) {
    callback(tileset, tile, *tile.content, tileTransform);
  }

  // 3D Tiles 1.1 multiple contents
  for (const Cesium3DTiles::Content& content : tile.contents) {
    callback(tileset, tile, content, tileTransform);
  }

  for (const Cesium3DTiles::Tile& childTile : tile.children) {
    forEachContentRecursive(tileTransform, tileset, childTile, callback);
  }
}

template <typename TCallback>
void forEachTileRecursive(
    const glm::dmat4& transform,
    const Tileset& tileset,
    const Tile& tile,
    TCallback& callback) {
  glm::dmat4 tileTransform =
      transform * getTileTransform(tile).value_or(glm::dmat4(1.0));

  callback(tileset, tile, tileTransform);

  for (const Cesium3DTiles::Tile& childTile : tile.children) {
    forEachTileRecursive(tileTransform, tileset, childTile, callback);
  }
}

} // namespace

void Tileset::forEachTile(std::function<ForEachTileCallback>&& callback) {
  return const_cast<const Tileset*>(this)->forEachTile(
      [&callback](
          const Tileset& tileset,
          const Tile& tile,
          const glm::dmat4& transform) {
        callback(
            const_cast<Tileset&>(tileset),
            const_cast<Tile&>(tile),
            transform);
      });
}

void Tileset::forEachTile(
    std::function<ForEachTileConstCallback>&& callback) const {
  forEachTileRecursive(glm::dmat4(1.0), *this, this->root, callback);
}

void Tileset::forEachContent(std::function<ForEachContentCallback>&& callback) {
  return const_cast<const Tileset*>(this)->forEachContent(
      [&callback](
          const Tileset& tileset,
          const Tile& tile,
          const Content& content,
          const glm::dmat4& transform) {
        callback(
            const_cast<Tileset&>(tileset),
            const_cast<Tile&>(tile),
            const_cast<Content&>(content),
            transform);
      });
}

void Tileset::forEachContent(
    std::function<ForEachContentConstCallback>&& callback) const {
  forEachContentRecursive(glm::dmat4(1.0), *this, this->root, callback);
}

void Tileset::addExtensionUsed(const std::string& extensionName) {
  if (!this->isExtensionUsed(extensionName)) {
    this->extensionsUsed.emplace_back(extensionName);
  }
}

void Tileset::addExtensionRequired(const std::string& extensionName) {
  this->addExtensionUsed(extensionName);

  if (!this->isExtensionRequired(extensionName)) {
    this->extensionsRequired.emplace_back(extensionName);
  }
}

void Tileset::removeExtensionUsed(const std::string& extensionName) {
  this->extensionsUsed.erase(
      std::remove(
          this->extensionsUsed.begin(),
          this->extensionsUsed.end(),
          extensionName),
      this->extensionsUsed.end());
}

void Tileset::removeExtensionRequired(const std::string& extensionName) {
  this->removeExtensionUsed(extensionName);

  this->extensionsRequired.erase(
      std::remove(
          this->extensionsRequired.begin(),
          this->extensionsRequired.end(),
          extensionName),
      this->extensionsRequired.end());
}

bool Tileset::isExtensionUsed(const std::string& extensionName) const noexcept {
  return std::find(
             this->extensionsUsed.begin(),
             this->extensionsUsed.end(),
             extensionName) != this->extensionsUsed.end();
}

bool Tileset::isExtensionRequired(
    const std::string& extensionName) const noexcept {
  return std::find(
             this->extensionsRequired.begin(),
             this->extensionsRequired.end(),
             extensionName) != this->extensionsRequired.end();
}

} // namespace Cesium3DTiles
