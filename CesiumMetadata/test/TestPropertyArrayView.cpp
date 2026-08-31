#include <CesiumGltf/PropertyArrayView.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace doctest;
using namespace CesiumGltf;

TEST_CASE("PropertyArrayCopy") {
  SUBCASE("bool") {
    std::vector<bool> values{
        true,
        false,
        true,
        true,
        false,
        true,
        false,
        false,
        true,
        false,
        true};

    SUBCASE("constructs from buffer") {
      PropertyArrayCopy<bool> array(values);
      REQUIRE_EQ(array.size(), values.size());
      for (int64_t i = 0; i < array.size(); i++) {
        CHECK_EQ(array[i], values[size_t(i)]);
      }
    }

    SUBCASE("constructs from copy") {
      PropertyArrayCopy<bool> array(values);
      PropertyArrayCopy<bool> copy(array);

      REQUIRE_EQ(copy.size(), values.size());
      for (int64_t i = 0; i < copy.size(); i++) {
        CHECK_EQ(copy[i], values[size_t(i)]);
      }

      CHECK_EQ(array.size(), values.size());
    }

    SUBCASE("constructs from move") {
      PropertyArrayCopy<bool> array(values);
      PropertyArrayCopy<bool> moved(std::move(array));

      REQUIRE_EQ(moved.size(), values.size());
      for (int64_t i = 0; i < moved.size(); i++) {
        CHECK_EQ(moved[i], values[size_t(i)]);
      }
    }

    SUBCASE("assigns from copy") {
      PropertyArrayCopy<bool> copy;
      PropertyArrayCopy<bool> array(values);
      copy = array;

      REQUIRE_EQ(copy.size(), values.size());
      for (int64_t i = 0; i < copy.size(); i++) {
        CHECK_EQ(copy[i], values[size_t(i)]);
      }

      CHECK_EQ(array.size(), values.size());
    }

    SUBCASE("assigns from move") {
      PropertyArrayCopy<bool> moved;
      PropertyArrayCopy<bool> array(values);
      moved = std::move(array);

      REQUIRE_EQ(moved.size(), values.size());
      for (int64_t i = 0; i < moved.size(); i++) {
        CHECK_EQ(moved[i], values[size_t(i)]);
      }
    }

    SUBCASE("toViewAndExternalBuffer") {
      PropertyArrayCopy<bool> array(values);

      std::vector<std::byte> storage;
      PropertyArrayView<bool> view =
          std::move(array).toViewAndExternalBuffer(storage);

      REQUIRE_EQ(view.size(), values.size());
      for (int64_t i = 0; i < view.size(); i++) {
        CHECK_EQ(view[i], values[size_t(i)]);
      }
    }
  }

  SUBCASE("scalar") {
    std::vector<int16_t> values{13, -4, -8, 17, 42, 6, 0, -9};

    SUBCASE("constructs from buffer") {
      PropertyArrayCopy<int16_t> array(values);
      REQUIRE_EQ(array.size(), values.size());
      for (int64_t i = 0; i < array.size(); i++) {
        CHECK_EQ(array[i], values[size_t(i)]);
      }
    }

    SUBCASE("constructs from copy") {
      PropertyArrayCopy<int16_t> array(values);
      PropertyArrayCopy<int16_t> copy(array);

      REQUIRE_EQ(copy.size(), values.size());
      for (int64_t i = 0; i < copy.size(); i++) {
        CHECK_EQ(copy[i], values[size_t(i)]);
      }

      CHECK_EQ(array.size(), values.size());
    }

    SUBCASE("constructs from move") {
      PropertyArrayCopy<int16_t> array(values);
      PropertyArrayCopy<int16_t> moved(std::move(array));

      REQUIRE_EQ(moved.size(), values.size());
      for (int64_t i = 0; i < moved.size(); i++) {
        CHECK_EQ(moved[i], values[size_t(i)]);
      }
    }

    SUBCASE("assigns from copy") {
      PropertyArrayCopy<int16_t> copy;
      PropertyArrayCopy<int16_t> array(values);
      copy = array;

      REQUIRE_EQ(copy.size(), values.size());
      for (int64_t i = 0; i < copy.size(); i++) {
        CHECK_EQ(copy[i], values[size_t(i)]);
      }

      CHECK_EQ(array.size(), values.size());
    }

    SUBCASE("assigns from move") {
      PropertyArrayCopy<int16_t> moved;
      PropertyArrayCopy<int16_t> array(values);
      moved = std::move(array);

      REQUIRE_EQ(moved.size(), values.size());
      for (int64_t i = 0; i < moved.size(); i++) {
        CHECK_EQ(moved[i], values[size_t(i)]);
      }
    }

    SUBCASE("toViewAndExternalBuffer") {
      PropertyArrayCopy<int16_t> array(values);

      std::vector<std::byte> storage;
      PropertyArrayView<int16_t> view =
          std::move(array).toViewAndExternalBuffer(storage);

      REQUIRE_EQ(view.size(), values.size());
      for (int64_t i = 0; i < view.size(); i++) {
        CHECK_EQ(view[i], values[size_t(i)]);
      }
    }
  }

  SUBCASE("string") {
    std::vector<std::string> values{
        "Lorem ipsum",
        "Test",
        "Somewhat longer test string",
        "The quick brown fox jumps over the lazy dog."};

    SUBCASE("constructs from buffer") {
      PropertyArrayCopy<std::string_view> array(values);
      REQUIRE_EQ(array.size(), values.size());
      for (int64_t i = 0; i < array.size(); i++) {
        CHECK_EQ(array[i], values[size_t(i)]);
      }
    }

    SUBCASE("constructs from copy") {
      PropertyArrayCopy<std::string_view> array(values);
      PropertyArrayCopy<std::string_view> copy(array);

      REQUIRE_EQ(copy.size(), values.size());
      for (int64_t i = 0; i < copy.size(); i++) {
        CHECK_EQ(copy[i], values[size_t(i)]);
      }

      CHECK_EQ(array.size(), values.size());
    }

    SUBCASE("constructs from move") {
      PropertyArrayCopy<std::string_view> array(values);
      PropertyArrayCopy<std::string_view> moved(std::move(array));

      REQUIRE_EQ(moved.size(), values.size());
      for (int64_t i = 0; i < moved.size(); i++) {
        CHECK_EQ(moved[i], values[size_t(i)]);
      }
    }

    SUBCASE("assigns from copy") {
      PropertyArrayCopy<std::string_view> copy;
      PropertyArrayCopy<std::string_view> array(values);
      copy = array;

      REQUIRE_EQ(copy.size(), values.size());
      for (int64_t i = 0; i < copy.size(); i++) {
        CHECK_EQ(copy[i], values[size_t(i)]);
      }

      CHECK_EQ(array.size(), values.size());
    }

    SUBCASE("assigns from move") {
      PropertyArrayCopy<std::string_view> moved;
      PropertyArrayCopy<std::string_view> array(values);
      moved = std::move(array);

      REQUIRE_EQ(moved.size(), values.size());
      for (int64_t i = 0; i < moved.size(); i++) {
        CHECK_EQ(moved[i], values[size_t(i)]);
      }
    }

    SUBCASE("toViewAndExternalBuffer") {
      PropertyArrayCopy<std::string_view> array(values);

      std::vector<std::byte> storage;
      PropertyArrayView<std::string_view> view =
          std::move(array).toViewAndExternalBuffer(storage);

      REQUIRE_EQ(view.size(), values.size());
      for (int64_t i = 0; i < view.size(); i++) {
        CHECK_EQ(view[i], values[size_t(i)]);
      }
    }
  }
}
