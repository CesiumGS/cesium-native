#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/Tile.h>

#include <doctest/doctest.h>

using namespace Cesium3DTilesSelection;

namespace {

std::vector<const Tile*> enumerate(const LoadedTileEnumerator& enumerator) {
  std::vector<const Tile*> result;

  for (const Tile& tile : enumerator) {
    result.push_back(&tile);
  }

  return result;
}

} // namespace

TEST_CASE("LoadedTileEnumerator") {
  Tile root(nullptr);

  std::vector<Tile> children;
  children.emplace_back(nullptr);
  children.emplace_back(nullptr);
  children.emplace_back(nullptr);

  root.createChildTiles(std::move(children));

  std::vector<Tile> grandchildren;
  grandchildren.emplace_back(nullptr);
  grandchildren.emplace_back(nullptr);
  grandchildren.emplace_back(nullptr);

  root.getChildren()[1].createChildTiles(std::move(grandchildren));

  LoadedTileEnumerator enumerator;
  enumerator.updateRootTile(&root);

  SUBCASE("with no loaded tiles it enumerates the root tile only") {
    CHECK(enumerate(enumerator) == std::vector<const Tile*>{&root});
  }

  SUBCASE("enumerates path to single tile") {
    root.getChildren()[1].getChildren()[2].incrementDoNotUnloadSubtreeCount(
        "test");
    CHECK(
        enumerate(enumerator) == std::vector<const Tile*>{
                                     &root,
                                     &root.getChildren()[1],
                                     &root.getChildren()[1].getChildren()[2]});
  }

  SUBCASE("enumerates complete tree") {
    root.getChildren()[0].incrementDoNotUnloadSubtreeCount("test");
    root.getChildren()[1].getChildren()[0].incrementDoNotUnloadSubtreeCount(
        "test");
    root.getChildren()[1].getChildren()[1].incrementDoNotUnloadSubtreeCount(
        "test");
    root.getChildren()[1].getChildren()[2].incrementDoNotUnloadSubtreeCount(
        "test");
    root.getChildren()[2].incrementDoNotUnloadSubtreeCount("test");
    CHECK(
        enumerate(enumerator) == std::vector<const Tile*>{
                                     &root,
                                     &root.getChildren()[0],
                                     &root.getChildren()[1],
                                     &root.getChildren()[1].getChildren()[0],
                                     &root.getChildren()[1].getChildren()[1],
                                     &root.getChildren()[1].getChildren()[2],
                                     &root.getChildren()[2]});
  }
}
