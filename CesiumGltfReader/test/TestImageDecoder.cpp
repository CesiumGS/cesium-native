#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumNativeTests/readFile.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumGltfReader;

TEST_CASE("CesiumGltfReader::ImageDecoder") {
  SUBCASE("Can correctly interpret mipmaps in KTX2 files") {
    {
      // This KTX2 file has a single mip level and no further mip levels should
      // be generated. `mipPositions` should reflect this single mip level.
      std::filesystem::path ktx2File = CesiumGltfReader_TEST_DATA_DIR;
      ktx2File /= "ktx2/kota-onelevel.ktx2";
      std::vector<std::byte> data = readFile(ktx2File.string());
      ImageReaderResult imageResult =
          ImageDecoder::readImage(data, Ktx2TranscodeTargets{});
      REQUIRE(imageResult.pImage);

      const ImageAsset& image = *imageResult.pImage;
      REQUIRE(image.mipPositions.size() == 1);
      CHECK(image.mipPositions[0].byteOffset == 0);
      CHECK(image.mipPositions[0].byteSize > 0);
      CHECK(
          image.mipPositions[0].byteSize ==
          size_t(image.width * image.height * image.channels));
      CHECK(image.mipPositions[0].byteSize == image.pixelData.size());
    }

    {
      // This KTX2 file has only a base image but further mip levels can be
      // generated. This image effectively has no mip levels.
      std::filesystem::path ktx2File = CesiumGltfReader_TEST_DATA_DIR;
      ktx2File /= "ktx2/kota-automipmap.ktx2";
      std::vector<std::byte> data = readFile(ktx2File.string());
      ImageReaderResult imageResult =
          ImageDecoder::readImage(data, Ktx2TranscodeTargets{});
      REQUIRE(imageResult.pImage);

      const ImageAsset& image = *imageResult.pImage;
      REQUIRE(image.mipPositions.size() == 0);
      CHECK(image.pixelData.size() > 0);
    }

    {
      // This KTX2 file has a complete mip chain.
      std::filesystem::path ktx2File = CesiumGltfReader_TEST_DATA_DIR;
      ktx2File /= "ktx2/kota-mipmaps.ktx2";
      std::vector<std::byte> data = readFile(ktx2File.string());
      ImageReaderResult imageResult =
          ImageDecoder::readImage(data, Ktx2TranscodeTargets{});
      REQUIRE(imageResult.pImage);

      const ImageAsset& image = *imageResult.pImage;
      REQUIRE(image.mipPositions.size() == 9);
      CHECK(image.mipPositions[0].byteSize > 0);
      CHECK(
          image.mipPositions[0].byteSize ==
          size_t(image.width * image.height * image.channels));
      CHECK(image.mipPositions[0].byteSize < image.pixelData.size());

      size_t smallerThan = image.mipPositions[0].byteSize;
      for (size_t i = 1; i < image.mipPositions.size(); ++i) {
        CHECK(image.mipPositions[i].byteSize < smallerThan);
        smallerThan = image.mipPositions[i].byteSize;
      }
    }
  }
}
