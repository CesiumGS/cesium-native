#include <CesiumGltf/ImageAsset.h>

#include <doctest/doctest.h>

TEST_CASE("ImageAsset::changeNumberOfChannels") {
  SUBCASE("Converts to fewer channels") {
    CesiumGltf::ImageAsset asset;
    asset.channels = 4;
    asset.width = 4;
    asset.height = 1;
    asset.bytesPerChannel = 1;
    asset.pixelData = {
        std::byte{0xff},
        std::byte{0xaa},
        std::byte{0x04},
        std::byte{0x02},
        std::byte{0xaa},
        std::byte{0xcc},
        std::byte{0x0d},
        std::byte{0x01},
        std::byte{0x99},
        std::byte{0x11},
        std::byte{0x0e},
        std::byte{0x0c},
        std::byte{0x9a},
        std::byte{0x9b},
        std::byte{0x01},
        std::byte{0x9b}};

    asset.changeNumberOfChannels(2);
    CHECK(asset.pixelData[0] == std::byte{0xff});
    CHECK(asset.pixelData[1] == std::byte{0xaa});
    CHECK(asset.pixelData[2] == std::byte{0xaa});
    CHECK(asset.pixelData[3] == std::byte{0xcc});
    CHECK(asset.pixelData[4] == std::byte{0x99});
    CHECK(asset.pixelData[5] == std::byte{0x11});
    CHECK(asset.pixelData[6] == std::byte{0x9a});
    CHECK(asset.pixelData[7] == std::byte{0x9b});
    CHECK(asset.channels == 2);
    CHECK(asset.pixelData.size() == 8);
  }

  SUBCASE("Converts to more channels") {
    CesiumGltf::ImageAsset asset;
    asset.channels = 1;
    asset.width = 4;
    asset.height = 1;
    asset.bytesPerChannel = 1;
    asset.pixelData =
        {std::byte{0xab}, std::byte{0xbc}, std::byte{0xcd}, std::byte{0xde}};

    asset.changeNumberOfChannels(2, std::byte{0x99});
    CHECK(asset.pixelData[0] == std::byte{0xab});
    CHECK(asset.pixelData[1] == std::byte{0x99});
    CHECK(asset.pixelData[2] == std::byte{0xbc});
    CHECK(asset.pixelData[3] == std::byte{0x99});
    CHECK(asset.pixelData[4] == std::byte{0xcd});
    CHECK(asset.pixelData[5] == std::byte{0x99});
    CHECK(asset.pixelData[6] == std::byte{0xde});
    CHECK(asset.pixelData[7] == std::byte{0x99});
    CHECK(asset.channels == 2);
    CHECK(asset.pixelData.size() == 8);
  }
}