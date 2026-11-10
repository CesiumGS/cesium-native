#include <CesiumImage/ImageAsset.h>
#include <CesiumImage/ImageDecoder.h>
#include <CesiumNativeTests/readFile.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>

using namespace CesiumImage;

TEST_CASE("CesiumImage::ImageDecoder") {
  SUBCASE("Can decode JPEG images") {
    std::filesystem::path jpegFile = CesiumImage_TEST_DATA_DIR;
    jpegFile /= "ktx2/kota.jpg";
    std::vector<std::byte> data = readFile(jpegFile.string());
    ImageReaderResult imageResult =
        ImageDecoder::readImage(data, Ktx2TranscodeTargets{});
    REQUIRE(imageResult.pImage);
    CHECK(imageResult.errors.empty());

    const ImageAsset& image = *imageResult.pImage;
    CHECK(image.width == 256);
    CHECK(image.height == 192);
    CHECK(image.channels == 4);
    CHECK(image.bytesPerChannel == 1);
    REQUIRE(image.pixelData.size() == 256 * 192 * 4);
    for (size_t i = 3; i < image.pixelData.size(); i += 4) {
      CHECK(image.pixelData[i] == std::byte{0xff});
    }
  }

  SUBCASE("Rejects JPEG images with truncated scan data") {
    std::filesystem::path jpegFile = CesiumImage_TEST_DATA_DIR;
    jpegFile /= "ktx2/kota.jpg";
    std::vector<std::byte> data = readFile(jpegFile.string());

    std::array startOfScan{std::byte{0xff}, std::byte{0xda}};
    auto scan = std::search(
        data.begin(),
        data.end(),
        startOfScan.begin(),
        startOfScan.end());
    REQUIRE(scan != data.end());
    REQUIRE(data.end() - scan >= 4);
    size_t headerLength = size_t(scan[2]) * 256 + size_t(scan[3]);
    size_t dataOffset = size_t(scan - data.begin()) + 2 + headerLength;
    REQUIRE(dataOffset <= data.size());
    // Keep the complete header so the decoder allocates its output buffer,
    // but omit the scan data so decompression fails.
    data.resize(dataOffset);

    ImageReaderResult imageResult =
        ImageDecoder::readImage(data, Ktx2TranscodeTargets{});
    CHECK_FALSE(imageResult.pImage);
    CHECK_FALSE(imageResult.errors.empty());
  }

  SUBCASE("Can correctly interpret mipmaps in KTX2 files") {
    {
      // This KTX2 file has a single mip level and no further mip levels should
      // be generated. `mipPositions` should reflect this single mip level.
      std::filesystem::path ktx2File = CesiumImage_TEST_DATA_DIR;
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
      std::filesystem::path ktx2File = CesiumImage_TEST_DATA_DIR;
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
      std::filesystem::path ktx2File = CesiumImage_TEST_DATA_DIR;
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
