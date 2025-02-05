#include <Cesium3DTiles/Tileset.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cstring>

using namespace Cesium3DTiles;

TEST_CASE("forEachTile") {
  Tileset tileset;

  Tile& root = tileset.root;

  root.children.emplace_back();
  root.children.emplace_back();

  Tile& child0 = root.children[0];
  Tile& child1 = root.children[1];

  Tile& grandchild = child0.children.emplace_back();

  glm::dmat4 rootTransform = glm::dmat4(2.0);
  glm::dmat4 child0Transform = glm::dmat4(3.0);
  glm::dmat4 child1Transform = glm::dmat4(4.0);
  glm::dmat4 grandchildTransform = glm::dmat4(5.0);

  std::memcpy(root.transform.data(), &rootTransform, sizeof(glm::dmat4));
  std::memcpy(child0.transform.data(), &child0Transform, sizeof(glm::dmat4));
  std::memcpy(child1.transform.data(), &child1Transform, sizeof(glm::dmat4));
  std::memcpy(
      grandchild.transform.data(),
      &grandchildTransform,
      sizeof(glm::dmat4));

  glm::dmat4 expectedRootTransform = rootTransform;
  glm::dmat4 expectedChild0Transform = rootTransform * child0Transform;
  glm::dmat4 expectedChild1Transform = rootTransform * child1Transform;
  glm::dmat4 expectedGrandchildTransform =
      rootTransform * child0Transform * grandchildTransform;

  std::vector<glm::dmat4> transforms;
  tileset.forEachTile(
      [&transforms](
          Tileset& /* tileset */,
          Tile& /* tile */,
          const glm::dmat4& transform) { transforms.push_back(transform); });

  REQUIRE(transforms.size() == 4);
  CHECK(transforms[0] == expectedRootTransform);
  CHECK(transforms[1] == expectedChild0Transform);
  CHECK(transforms[2] == expectedGrandchildTransform);
  CHECK(transforms[3] == expectedChild1Transform);
}

TEST_CASE("forEachContent") {
  Tileset tileset;

  Tile& root = tileset.root;

  root.children.emplace_back();
  root.children.emplace_back();

  Tile& child0 = root.children[0];
  Tile& child1 = root.children[1];

  Tile& grandchild = child0.children.emplace_back();

  const Content& rootContent = root.content.emplace();
  const Content& child1Content = child1.content.emplace();

  grandchild.contents.emplace_back();
  grandchild.contents.emplace_back();

  const Content& grandchildContent0 = grandchild.contents[0];
  const Content& grandchildContent1 = grandchild.contents[1];

  glm::dmat4 rootTransform = glm::dmat4(2.0);
  glm::dmat4 child0Transform = glm::dmat4(3.0);
  glm::dmat4 child1Transform = glm::dmat4(4.0);
  glm::dmat4 grandchildTransform = glm::dmat4(5.0);

  std::memcpy(root.transform.data(), &rootTransform, sizeof(glm::dmat4));
  std::memcpy(child0.transform.data(), &child0Transform, sizeof(glm::dmat4));
  std::memcpy(child1.transform.data(), &child1Transform, sizeof(glm::dmat4));
  std::memcpy(
      grandchild.transform.data(),
      &grandchildTransform,
      sizeof(glm::dmat4));

  glm::dmat4 expectedRootTransform = rootTransform;
  glm::dmat4 expectedChild1Transform = rootTransform * child1Transform;
  glm::dmat4 expectedGrandchildTransform =
      rootTransform * child0Transform * grandchildTransform;

  std::vector<glm::dmat4> transforms;
  std::vector<Cesium3DTiles::Content*> contents;
  tileset.forEachContent([&transforms, &contents](
                             Tileset& /* tileset */,
                             Tile& /* tile */,
                             Content& content,
                             const glm::dmat4& transform) {
    transforms.push_back(transform);
    contents.push_back(&content);
  });

  REQUIRE(transforms.size() == 4);
  CHECK(transforms[0] == expectedRootTransform);
  CHECK(transforms[1] == expectedGrandchildTransform);
  CHECK(transforms[2] == expectedGrandchildTransform);
  CHECK(transforms[3] == expectedChild1Transform);

  REQUIRE(contents.size() == 4);
  CHECK(contents[0] == &rootContent);
  CHECK(contents[1] == &grandchildContent0);
  CHECK(contents[2] == &grandchildContent1);
  CHECK(contents[3] == &child1Content);
}

TEST_CASE("addExtensionUsed") {
  SUBCASE("adds a new extension") {
    Tileset tileset;

    tileset.addExtensionUsed("Foo");
    tileset.addExtensionUsed("Bar");

    CHECK(tileset.extensionsUsed.size() == 2);
    CHECK(
        std::find(
            tileset.extensionsUsed.begin(),
            tileset.extensionsUsed.end(),
            "Foo") != tileset.extensionsUsed.end());
    CHECK(
        std::find(
            tileset.extensionsUsed.begin(),
            tileset.extensionsUsed.end(),
            "Bar") != tileset.extensionsUsed.end());
  }

  SUBCASE("does not add a duplicate extension") {
    Tileset tileset;

    tileset.addExtensionUsed("Foo");
    tileset.addExtensionUsed("Bar");
    tileset.addExtensionUsed("Foo");

    CHECK(tileset.extensionsUsed.size() == 2);
    CHECK(
        std::find(
            tileset.extensionsUsed.begin(),
            tileset.extensionsUsed.end(),
            "Foo") != tileset.extensionsUsed.end());
    CHECK(
        std::find(
            tileset.extensionsUsed.begin(),
            tileset.extensionsUsed.end(),
            "Bar") != tileset.extensionsUsed.end());
  }

  SUBCASE("does not also add the extension to extensionsRequired") {
    Tileset tileset;
    tileset.addExtensionUsed("Foo");
    CHECK(tileset.extensionsRequired.empty());
  }
}

TEST_CASE("addExtensionRequired") {
  SUBCASE("adds a new extension") {
    Tileset tileset;

    tileset.addExtensionRequired("Foo");
    tileset.addExtensionRequired("Bar");

    CHECK(tileset.extensionsRequired.size() == 2);
    CHECK(
        std::find(
            tileset.extensionsRequired.begin(),
            tileset.extensionsRequired.end(),
            "Foo") != tileset.extensionsRequired.end());
    CHECK(
        std::find(
            tileset.extensionsRequired.begin(),
            tileset.extensionsRequired.end(),
            "Bar") != tileset.extensionsRequired.end());
  }

  SUBCASE("does not add a duplicate extension") {
    Tileset tileset;

    tileset.addExtensionRequired("Foo");
    tileset.addExtensionRequired("Bar");
    tileset.addExtensionRequired("Foo");

    CHECK(tileset.extensionsRequired.size() == 2);
    CHECK(
        std::find(
            tileset.extensionsRequired.begin(),
            tileset.extensionsRequired.end(),
            "Foo") != tileset.extensionsRequired.end());
    CHECK(
        std::find(
            tileset.extensionsRequired.begin(),
            tileset.extensionsRequired.end(),
            "Bar") != tileset.extensionsRequired.end());
  }

  SUBCASE("also adds the extension to extensionsUsed if not already present") {
    Tileset tileset;

    tileset.addExtensionUsed("Bar");
    tileset.addExtensionRequired("Foo");
    tileset.addExtensionRequired("Bar");

    CHECK(tileset.extensionsUsed.size() == 2);
    CHECK(
        std::find(
            tileset.extensionsUsed.begin(),
            tileset.extensionsUsed.end(),
            "Foo") != tileset.extensionsUsed.end());
    CHECK(
        std::find(
            tileset.extensionsUsed.begin(),
            tileset.extensionsUsed.end(),
            "Bar") != tileset.extensionsUsed.end());
  }
}

TEST_CASE("removeExtensionUsed") {
  SUBCASE("removes an extension") {
    Tileset tileset;
    tileset.extensionsUsed = {"Foo", "Bar"};

    tileset.removeExtensionUsed("Foo");

    CHECK(tileset.extensionsUsed == std::vector<std::string>{"Bar"});

    tileset.removeExtensionUsed("Bar");

    CHECK(tileset.extensionsUsed.empty());

    tileset.removeExtensionUsed("Other");

    CHECK(tileset.extensionsUsed.empty());
  }

  SUBCASE("does not also remove the extension from extensionsRequired") {
    Tileset tileset;
    tileset.extensionsUsed = {"Foo"};
    tileset.extensionsRequired = {"Foo"};

    tileset.removeExtensionUsed("Foo");

    CHECK(tileset.extensionsUsed.empty());
    CHECK(!tileset.extensionsRequired.empty());
  }
}

TEST_CASE("removeExtensionRequired") {
  SUBCASE("removes an extension") {
    Tileset tileset;
    tileset.extensionsRequired = {"Foo", "Bar"};

    tileset.removeExtensionRequired("Foo");

    CHECK(tileset.extensionsRequired == std::vector<std::string>{"Bar"});

    tileset.removeExtensionRequired("Bar");

    CHECK(tileset.extensionsRequired.empty());

    tileset.removeExtensionRequired("Other");

    CHECK(tileset.extensionsRequired.empty());
  }

  SUBCASE("also removes the extension from extensionsUsed if present") {
    Tileset tileset;
    tileset.extensionsUsed = {"Foo"};
    tileset.extensionsRequired = {"Foo"};

    tileset.removeExtensionRequired("Foo");

    CHECK(tileset.extensionsUsed.empty());
    CHECK(tileset.extensionsRequired.empty());
  }
}

TEST_CASE("isExtensionUsed") {
  Tileset tileset;
  tileset.extensionsUsed = {"Foo", "Bar"};

  CHECK(tileset.isExtensionUsed("Foo"));
  CHECK(tileset.isExtensionUsed("Bar"));
  CHECK_FALSE(tileset.isExtensionUsed("Baz"));
}

TEST_CASE("isExtensionRequired") {
  Tileset tileset;
  tileset.extensionsRequired = {"Foo", "Bar"};

  CHECK(tileset.isExtensionRequired("Foo"));
  CHECK(tileset.isExtensionRequired("Bar"));
  CHECK_FALSE(tileset.isExtensionRequired("Baz"));
}
