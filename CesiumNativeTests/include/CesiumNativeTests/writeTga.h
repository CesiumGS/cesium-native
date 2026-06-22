#pragma once

#include <filesystem>

namespace CesiumImage {
struct ImageAsset;
}

namespace CesiumNativeTests {
/**
 * @brief Writes the provided image to a TGA file. If the image has any mip
 * levels,
 * they will be written as `filename-mip{level}.tga`, where `{level}` is the mip level.
 *
 * This method is only meant to be used for debugging. It does not support any
 * compressed pixel formats.
 */
void writeImageToTgaFile(
    const CesiumImage::ImageAsset& image,
    const std::filesystem::path& outputPath);
} // namespace CesiumNativeTests
