#include <CesiumUtility/TreeTraversalState.h>

#include <doctest/doctest.h>

#include <string>

using namespace CesiumUtility;

namespace {

struct Node {
  std::string name;
};

} // namespace

TEST_CASE("TreeTraversalState") {
  SUBCASE("Three Levels") {
    Node a{"a"}, b{"b"}, c{"c"}, d{"d"}, e{"e"}, f{"f"};
    TreeTraversalState<Node*, int> traversalState;

    /*
     *       a
     *     / | \
     *    b  c  d
     *      / \
     *     e   f
     */
    // clang-format off
    traversalState.beginNode(&a);
      traversalState.currentState() = 1;

      traversalState.beginNode(&b);
        traversalState.currentState() = 2;
      traversalState.finishNode(&b);

      traversalState.beginNode(&c);
        traversalState.currentState() = 3;

        traversalState.beginNode(&e);
          traversalState.currentState() = 5;
        traversalState.finishNode(&e);

        traversalState.beginNode(&f);
          traversalState.currentState() = 6;
        traversalState.finishNode(&f);

      traversalState.finishNode(&c);

      traversalState.beginNode(&d);
        traversalState.currentState() = 4;
      traversalState.finishNode(&d);

    traversalState.finishNode(&a);
    // clang-format on

    SUBCASE("First traversal captures current states") {
      auto map = traversalState.slowlyGetCurrentStates();
      CHECK(map[&a] == 1);
      CHECK(map[&b] == 2);
      CHECK(map[&c] == 3);
      CHECK(map[&d] == 4);
      CHECK(map[&e] == 5);
      CHECK(map[&f] == 6);
    }

    SUBCASE("First traversal does not populate previous") {
      CHECK(traversalState.slowlyGetPreviousStates().empty());
    }

    SUBCASE("beginTraversal moves current to previous") {
      traversalState.beginTraversal();
      CHECK(!traversalState.slowlyGetPreviousStates().empty());
      CHECK(traversalState.slowlyGetCurrentStates().empty());
    }

    SUBCASE("Second identical traversal can access previous states") {
      traversalState.beginTraversal();

      // clang-format off
      traversalState.beginNode(&a);
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);

        traversalState.beginNode(&b);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 2);
        traversalState.finishNode(&b);

        traversalState.beginNode(&c);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);

          traversalState.beginNode(&e);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 5);
          traversalState.finishNode(&e);

          traversalState.beginNode(&f);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 6);
          traversalState.finishNode(&f);

        traversalState.finishNode(&c);

        traversalState.beginNode(&d);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

      traversalState.finishNode(&a);
      // clang-format on
    }

    SUBCASE("Second traversal can skip children") {
      traversalState.beginTraversal();

      // clang-format off
      traversalState.beginNode(&a);
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);

        traversalState.beginNode(&b);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 2);
        traversalState.finishNode(&b);

        traversalState.beginNode(&c);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);
        traversalState.finishNode(&c);

        traversalState.beginNode(&d);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

      traversalState.finishNode(&a);
      // clang-format on
    }

    SUBCASE("Second traversal can visit new children") {
      Node g{"g"};

      traversalState.beginTraversal();

      // clang-format off
      traversalState.beginNode(&a);
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);

        traversalState.beginNode(&b);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 2);
        traversalState.finishNode(&b);

        traversalState.beginNode(&c);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);

          traversalState.beginNode(&e);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 5);

            traversalState.beginNode(&g);
              CHECK(traversalState.previousState() == nullptr);
              traversalState.currentState() = 7;
            traversalState.finishNode(&g);

          traversalState.finishNode(&e);

          traversalState.beginNode(&f);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 6);
          traversalState.finishNode(&f);

        traversalState.finishNode(&c);

        traversalState.beginNode(&d);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

      traversalState.finishNode(&a);
      // clang-format on
    }

    SUBCASE("Second traversal can add two new levels") {
      Node g{"g"}, h{"h"};

      traversalState.beginTraversal();

      // clang-format off
      traversalState.beginNode(&a);
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);

        traversalState.beginNode(&b);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 2);
        traversalState.finishNode(&b);

        traversalState.beginNode(&c);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);

          traversalState.beginNode(&e);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 5);

            traversalState.beginNode(&g);
              CHECK(traversalState.previousState() == nullptr);
              traversalState.currentState() = 7;

              traversalState.beginNode(&h);
                CHECK(traversalState.previousState() == nullptr);
                traversalState.currentState() = 8;
              traversalState.finishNode(&h);
            traversalState.finishNode(&g);

          traversalState.finishNode(&e);

          traversalState.beginNode(&f);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 6);
          traversalState.finishNode(&f);

        traversalState.finishNode(&c);

        traversalState.beginNode(&d);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

      traversalState.finishNode(&a);
      // clang-format on
    }

    SUBCASE(
        "Can access both current and previous states after child finishNode") {
      traversalState.beginTraversal();

      // clang-format off
      traversalState.beginNode(&a);
        traversalState.currentState() = 1;

        traversalState.beginNode(&b);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 2);
        traversalState.finishNode(&b);

        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);
        CHECK(traversalState.currentState() == 1);
        traversalState.currentState() = 100;
        CHECK(traversalState.currentState() == 100);

        traversalState.beginNode(&c);
          traversalState.currentState() = 3;

          traversalState.beginNode(&e);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 5);
          traversalState.finishNode(&e);

          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);
          CHECK(traversalState.currentState() == 3);
          traversalState.currentState() = 300;
          CHECK(traversalState.currentState() == 300);

          traversalState.beginNode(&f);
            REQUIRE(traversalState.previousState() != nullptr);
            CHECK(*traversalState.previousState() == 6);
          traversalState.finishNode(&f);

          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);
          CHECK(traversalState.currentState() == 300);
          traversalState.currentState() = 350;
          CHECK(traversalState.currentState() == 350);
        traversalState.finishNode(&c);

        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);
        CHECK(traversalState.currentState() == 100);
        traversalState.currentState() = 150;
        CHECK(traversalState.currentState() == 150);

        traversalState.beginNode(&d);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);
        CHECK(traversalState.currentState() == 150);
        traversalState.currentState() = 175;
        CHECK(traversalState.currentState() == 175);
      traversalState.finishNode(&a);
      // clang-format on
    }
  }

  SUBCASE("Four Levels") {
    Node a{"a"}, b{"b"}, c{"c"}, d{"d"}, e{"e"}, f{"f"}, g{"g"}, h{"h"};
    TreeTraversalState<Node*, int> traversalState;

    /*
     *       a
     *     / | \
     *    b  c  d
     *      / \
     *     e   f
     *    / \
     *   g   h
     */
    // clang-format off
    traversalState.beginNode(&a);
      traversalState.currentState() = 1;

      traversalState.beginNode(&b);
        traversalState.currentState() = 2;
      traversalState.finishNode(&b);

      traversalState.beginNode(&c);
        traversalState.currentState() = 3;

        traversalState.beginNode(&e);
          traversalState.currentState() = 5;

          traversalState.beginNode(&g);
            CHECK(traversalState.previousState() == nullptr);
            traversalState.currentState() = 7;
          traversalState.finishNode(&g);

          traversalState.beginNode(&h);
            CHECK(traversalState.previousState() == nullptr);
            traversalState.currentState() = 8;
          traversalState.finishNode(&h);
        traversalState.finishNode(&e);

        traversalState.beginNode(&f);
          traversalState.currentState() = 6;
        traversalState.finishNode(&f);

      traversalState.finishNode(&c);

      traversalState.beginNode(&d);
        traversalState.currentState() = 4;
      traversalState.finishNode(&d);

    traversalState.finishNode(&a);
    // clang-format on

    SUBCASE("Second traversal can skip two levels") {
      traversalState.beginTraversal();

      // clang-format off
      traversalState.beginNode(&a);
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 1);

        traversalState.beginNode(&b);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 2);
        traversalState.finishNode(&b);

        traversalState.beginNode(&c);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 3);
        traversalState.finishNode(&c);

        traversalState.beginNode(&d);
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

      traversalState.finishNode(&a);
      // clang-format on
    }
  }

  SUBCASE("Node with children after a node where they're added") {
    Node a{"a"}, b{"b"}, c{"c"}, d{"d"}, e{"e"}, f{"f"}, g{"g"};
    TreeTraversalState<Node*, int> traversalState;

    /*
     * First traversal:
     *      a
     *    /  \
     *   b    c
     *       / \
     *      d   e
     */

    /*
     * Second traversal:
     *         a
     *       /  \
     *      b    c
     *     /    / \
     *    f    d   e
     *   /
     *  g
     */

    traversalState.beginTraversal();

    // clang-format off
    traversalState.beginNode(&a);
      traversalState.currentState() = 1;

      traversalState.beginNode(&b);
        traversalState.currentState() = 2;
      traversalState.finishNode(&b);

      traversalState.beginNode(&c);
        traversalState.currentState() = 3;

        traversalState.beginNode(&d);
          traversalState.currentState() = 4;
        traversalState.finishNode(&d);

        traversalState.beginNode(&e);
          traversalState.currentState() = 5;
        traversalState.finishNode(&e);

      traversalState.finishNode(&c);

    traversalState.finishNode(&a);
    // clang-format on

    traversalState.beginTraversal();

    // clang-format off
    traversalState.beginNode(&a);
      traversalState.currentState() = 1;
      REQUIRE(traversalState.previousState() != nullptr);
      CHECK(*traversalState.previousState() == 1);

      traversalState.beginNode(&b);
        traversalState.currentState() = 2;
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 2);

        traversalState.beginNode(&f);
          traversalState.currentState() = 6;
          REQUIRE(traversalState.previousState() == nullptr);

          traversalState.beginNode(&g);
            traversalState.currentState() = 7;
            REQUIRE(traversalState.previousState() == nullptr);
          traversalState.finishNode(&g);
        traversalState.finishNode(&f);

      traversalState.finishNode(&b);

      traversalState.beginNode(&c);
        traversalState.currentState() = 3;
        REQUIRE(traversalState.previousState() != nullptr);
        CHECK(*traversalState.previousState() == 3);

        traversalState.beginNode(&d);
          traversalState.currentState() = 4;
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 4);
        traversalState.finishNode(&d);

        traversalState.beginNode(&e);
          traversalState.currentState() = 5;
          REQUIRE(traversalState.previousState() != nullptr);
          CHECK(*traversalState.previousState() == 5);
        traversalState.finishNode(&e);

      traversalState.finishNode(&c);

    traversalState.finishNode(&a);
    // clang-format on
  }
}
