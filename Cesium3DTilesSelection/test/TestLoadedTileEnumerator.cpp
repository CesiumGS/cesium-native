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

  LoadedTileEnumerator enumerator(&root);

  SUBCASE("with no loaded tiles it enumerates nothing") {
    CHECK(enumerate(enumerator) == std::vector<const Tile*>{});
  }

  SUBCASE("enumerates path to single tile") {
    Tile::Pointer pTile2 = &root.getChildren()[1].getChildren()[2];
    CHECK(
        enumerate(enumerator) == std::vector<const Tile*>{
                                     &root,
                                     &root.getChildren()[1],
                                     &root.getChildren()[1].getChildren()[2]});
  }

  SUBCASE("enumerates complete tree") {
    Tile::Pointer pTile0 = &root.getChildren()[0];
    Tile::Pointer pTile10 = &root.getChildren()[1].getChildren()[0];
    Tile::Pointer pTile11 = &root.getChildren()[1].getChildren()[1];
    Tile::Pointer pTile12 = &root.getChildren()[1].getChildren()[2];
    Tile::Pointer pTile2 = &root.getChildren()[2];
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
