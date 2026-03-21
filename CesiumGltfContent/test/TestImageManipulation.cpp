#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfContent/ImageManipulation.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumGltfContent;

TEST_CASE("ImageManipulation::unsafeBlitImage entire image") {
  size_t width = 10;
  size_t height = 10;
  size_t imagePixels = width * height;
  size_t bufferPixels = 2;
  size_t bytesPerPixel = 2;

  std::vector<std::byte> target(
      (imagePixels + bufferPixels) * bytesPerPixel,
      std::byte(1));
  std::vector<std::byte> source(imagePixels * bytesPerPixel, std::byte(2));
  ImageManipulation::unsafeBlitImage(
      target.data() + bytesPerPixel,
      width * bytesPerPixel,
      source.data(),
      width * bytesPerPixel,
      width,
      height,
      bytesPerPixel);

  // Verify we haven't overflowed the target
  CHECK(target[0] == std::byte(1));
  CHECK(target[1] == std::byte(1));
  CHECK(target[target.size() - 1] == std::byte(1));
  CHECK(target[target.size() - 2] == std::byte(1));

  // Verify we did copy the expected bytes
  for (size_t i = 2; i < target.size() - 2; ++i) {
    CHECK(target[i] == std::byte(2));
  }
}

TEST_CASE("ImageManipulation::unsafeBlitImage subset of target") {
  size_t targetWidth = 10;
  size_t targetHeight = 10;
  size_t targetImagePixels = targetWidth * targetHeight;
  size_t bufferPixels = 2;
  size_t bytesPerPixel = 2;

  size_t sourceWidth = 4;
  size_t sourceHeight = 7;
  size_t sourceImagePixels = sourceWidth * sourceHeight;

  std::vector<std::byte> target(
      (targetImagePixels + bufferPixels) * bytesPerPixel,
      std::byte(1));
  std::vector<std::byte> source(
      sourceImagePixels * bytesPerPixel,
      std::byte(2));
  ImageManipulation::unsafeBlitImage(
      target.data() + bytesPerPixel,
      targetWidth * bytesPerPixel,
      source.data(),
      sourceWidth * bytesPerPixel,
      sourceWidth,
      sourceHeight,
      bytesPerPixel);

  // Verify we haven't overflowed the target
  CHECK(target[0] == std::byte(1));
  CHECK(target[1] == std::byte(1));
  CHECK(target[target.size() - 1] == std::byte(1));
  CHECK(target[target.size() - 2] == std::byte(1));

  // Verify we did copy the expected bytes
  for (size_t j = 0; j < targetHeight; ++j) {
    for (size_t i = 0; i < targetWidth; ++i) {
      std::byte expected =
          i < sourceWidth && j < sourceHeight ? std::byte(2) : std::byte(1);
      CHECK(target[(1 + j * targetWidth + i) * bytesPerPixel] == expected);
      CHECK(target[(1 + j * targetWidth + i) * bytesPerPixel + 1] == expected);
    }
  }
}

TEST_CASE("ImageManipulation::unsafeBlitImage subset of source") {
  size_t targetWidth = 10;
  size_t targetHeight = 10;
  size_t targetImagePixels = targetWidth * targetHeight;
  size_t bufferPixels = 2;
  size_t bytesPerPixel = 2;

  size_t sourceTotalWidth = 12;
  size_t sourceWidth = 4;
  size_t sourceHeight = 7;
  size_t sourceImagePixels = sourceTotalWidth * sourceHeight;

  std::vector<std::byte> target(
      (targetImagePixels + bufferPixels) * bytesPerPixel,
      std::byte(1));
  std::vector<std::byte> source(
      sourceImagePixels * bytesPerPixel,
      std::byte(2));
  ImageManipulation::unsafeBlitImage(
      target.data() + bytesPerPixel,
      targetWidth * bytesPerPixel,
      source.data(),
      sourceTotalWidth * bytesPerPixel,
      sourceWidth,
      sourceHeight,
      bytesPerPixel);

  // Verify we haven't overflowed the target
  CHECK(target[0] == std::byte(1));
  CHECK(target[1] == std::byte(1));
  CHECK(target[target.size() - 1] == std::byte(1));
  CHECK(target[target.size() - 2] == std::byte(1));

  // Verify we did copy the expected bytes
  for (size_t j = 0; j < targetHeight; ++j) {
    for (size_t i = 0; i < targetWidth; ++i) {
      std::byte expected =
          i < sourceWidth && j < sourceHeight ? std::byte(2) : std::byte(1);
      CHECK(target[(1 + j * targetWidth + i) * bytesPerPixel] == expected);
      CHECK(target[(1 + j * targetWidth + i) * bytesPerPixel + 1] == expected);
    }
  }
}

TEST_CASE("ImageManipulation::blitImage") {
  ImageAsset target;
  target.bytesPerChannel = 2;
  target.channels = 4;
  target.width = 15;
  target.height = 9;
  target.pixelData = std::vector<std::byte>(
      size_t(
          target.width * target.height * target.channels *
          target.bytesPerChannel),
      std::byte(1));

  ImageAsset source;
  source.bytesPerChannel = 2;
  source.channels = 4;
  source.width = 10;
  source.height = 11;
  source.pixelData = std::vector<std::byte>(
      size_t(
          source.width * source.height * source.channels *
          source.bytesPerChannel),
      std::byte(2));

  PixelRectangle sourceRect;
  sourceRect.x = 1;
  sourceRect.y = 2;
  sourceRect.width = 3;
  sourceRect.height = 4;

  PixelRectangle targetRect;
  targetRect.x = 6;
  targetRect.y = 5;
  targetRect.width = 3;
  targetRect.height = 4;

  auto verifyTargetUnchanged = [&target]() {
    CHECK(std::all_of(
        target.pixelData.begin(),
        target.pixelData.end(),
        [](std::byte b) { return b == std::byte(1); }));
  };

  auto contains = [](const PixelRectangle& rectangle, size_t x, size_t y) {
    if (x < size_t(rectangle.x)) {
      return false;
    }
    if (y < size_t(rectangle.y)) {
      return false;
    }
    if (x >= size_t(rectangle.x + rectangle.width)) {
      return false;
    }
    if (y >= size_t(rectangle.y + rectangle.height)) {
      return false;
    }
    return true;
  };

  auto verifySuccessfulCopy = [&target, &targetRect, &contains]() {
    size_t bytesPerPixel = size_t(target.bytesPerChannel * target.channels);

    for (size_t j = 0; j < size_t(target.height); ++j) {
      for (size_t i = 0; i < size_t(target.width); ++i) {
        size_t index = j * size_t(target.width) + i;
        size_t offset = index * bytesPerPixel;
        std::byte expected =
            contains(targetRect, i, j) ? std::byte(2) : std::byte(1);
        for (size_t k = 0; k < bytesPerPixel; ++k) {
          CHECK(target.pixelData[offset + k] == expected);
        }
      }
    }
  };

  SUBCASE("succeeds for non-scaled blit") {
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        true);
    verifySuccessfulCopy();
  }

  SUBCASE("succeeds for a scaled-up blit") {
    // Resizing is currently only supported for images that use one byte per
    // channel.
    target.bytesPerChannel = 1;
    source.bytesPerChannel = 1;

    targetRect.y = 4;
    targetRect.width = 4;
    targetRect.height = 5;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        true);
    verifySuccessfulCopy();
  }

  SUBCASE("succeeds for a scaled-down blit") {
    // Resizing is currently only supported for images that use one byte per
    // channel.
    target.bytesPerChannel = 1;
    source.bytesPerChannel = 1;

    targetRect.width = 2;
    targetRect.height = 3;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        true);
    verifySuccessfulCopy();
  }

  SUBCASE("returns false for mismatched bytesPerChannel") {
    target.bytesPerChannel = 1;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false for mismatched channels") {
    target.channels = 3;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when target X is outside target image") {
    targetRect.x = 14;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when target Y is outside target image") {
    targetRect.y = 6;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when target X is negative") {
    targetRect.x = -1;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when target Y is negative") {
    targetRect.y = -1;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when source X is outside source image") {
    sourceRect.x = 9;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when source Y is outside source image") {
    sourceRect.y = 9;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when source X is negative") {
    sourceRect.x = -1;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false when source Y is negative") {
    sourceRect.y = -1;
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }

  SUBCASE("returns false for a too-small target") {
    target.pixelData.resize(10);
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
  }

  SUBCASE("returns false for a too-small source") {
    source.pixelData.resize(10);
    CHECK(
        ImageManipulation::blitImage(target, targetRect, source, sourceRect) ==
        false);
    verifyTargetUnchanged();
  }
}
