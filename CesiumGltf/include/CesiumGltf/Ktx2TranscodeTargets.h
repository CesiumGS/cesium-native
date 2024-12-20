#pragma once

#include <CesiumGltf/Library.h>

namespace CesiumGltf {

/**
 * @brief Supported GPU compressed pixel formats.
 */
enum class CESIUMGLTF_API GpuCompressedPixelFormat {
  /**
   * @brief The data is uncompressed.
   */
  NONE,
  /**
   * @brief The data is an <a
   * href="https://registry.khronos.org/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt">ETC1-compressed</a>
   * RGB888 texture.
   */
  ETC1_RGB,
  /**
   * @brief The data is an ETC2-compressed RGBA8888 texture.
   */
  ETC2_RGBA,
  /**
   * @brief The data is a <a
   * href="https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc1">BC1-compressed</a>
   * RGB565 texture.
   */
  BC1_RGB,
  /**
   * @brief The data is a <a
   * href="https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc3">BC3-compressed</a>
   * RGBA5658 texture.
   */
  BC3_RGBA,
  /**
   * @brief The data is a <a
   * href="https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc4">BC4-compressed</a>
   * R8 texture.
   */
  BC4_R,
  /**
   * @brief The data is a <a
   * href="https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc5">BC5-compressed</a>
   * RG88 texture.
   */
  BC5_RG,
  /**
   * @brief The data is a <a
   * href="https://learn.microsoft.com/en-us/windows/win32/direct3d11/bc7-format">BC7-compressed</a>
   * RGBA8888 texture.
   */
  BC7_RGBA,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#_format_pvrtc1_4bpp">PVRTC1-compressed</a>
   * RGB444 texture.
   */
  PVRTC1_4_RGB,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#_format_pvrtc1_4bpp">PVRTC1-compressed</a>
   * RGBA4444 texture.
   */
  PVRTC1_4_RGBA,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.1/dataformat.1.1.html#ASTC">ASTC-compressed</a>
   * RGBA texture with a 4x4 block footprint.
   */
  ASTC_4x4_RGBA,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#_format_pvrtc2_4bpp">PVRTC2-compressed</a>
   * RGB444 texture.
   */
  PVRTC2_4_RGB,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#_format_pvrtc2_4bpp">PVRTC2-compressed</a>
   * RGBA4444 texture.
   */
  PVRTC2_4_RGBA,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#Section-r11eac">ETC2
   * R11 EAC-compressed</a> texture with a single channel.
   */
  ETC2_EAC_R11,
  /**
   * @brief The data is a <a
   * href="https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#Section-rg11eac">ETC2
   * RG11 EAC-compressed</a> texture with two channels.
   */
  ETC2_EAC_RG11
};

/**
 * @brief Represents the status of support for all
 * {@link GpuCompressedPixelFormat}s on a client platform.
 *
 * Clients can use this struct to convey which gpu compressed pixel formats are
 * supported. This can be used to construct a {@link Ktx2TranscodeTargets}.
 *
 * Each entry in the struct is a bool that represents whether the gpu
 * compressed pixel format with the corresponding name is supported.
 */
struct CESIUMGLTF_API SupportedGpuCompressedPixelFormats {
  /** @copydoc GpuCompressedPixelFormat::ETC1_RGB */
  bool ETC1_RGB{};
  /** @copydoc GpuCompressedPixelFormat::ETC2_RGBA */
  bool ETC2_RGBA{};
  /** @copydoc GpuCompressedPixelFormat::BC1_RGB */
  bool BC1_RGB{};
  /** @copydoc GpuCompressedPixelFormat::BC3_RGBA */
  bool BC3_RGBA{};
  /** @copydoc GpuCompressedPixelFormat::BC4_R */
  bool BC4_R{};
  /** @copydoc GpuCompressedPixelFormat::BC5_RG */
  bool BC5_RG{};
  /** @copydoc GpuCompressedPixelFormat::BC7_RGBA */
  bool BC7_RGBA{};
  /** @copydoc GpuCompressedPixelFormat::PVRTC1_4_RGB */
  bool PVRTC1_4_RGB{};
  /** @copydoc GpuCompressedPixelFormat::PVRTC1_4_RGBA */
  bool PVRTC1_4_RGBA{};
  /** @copydoc GpuCompressedPixelFormat::ASTC_4x4_RGBA */
  bool ASTC_4x4_RGBA{};
  /** @copydoc GpuCompressedPixelFormat::PVRTC2_4_RGB */
  bool PVRTC2_4_RGB{};
  /** @copydoc GpuCompressedPixelFormat::PVRTC2_4_RGBA */
  bool PVRTC2_4_RGBA{};
  /** @copydoc GpuCompressedPixelFormat::ETC2_EAC_R11 */
  bool ETC2_EAC_R11{};
  /** @copydoc GpuCompressedPixelFormat::ETC2_EAC_RG11 */
  bool ETC2_EAC_RG11{};
};

/**
 * @brief For each possible input transmission format, this struct names
 * the ideal target gpu-compressed pixel format to transcode to.
 *
 * When built with the constructor, these targets can take into account
 * platform-specific support for target formats as reported by the client.
 */
struct CESIUMGLTF_API Ktx2TranscodeTargets {

  /**
   * @brief The gpu pixel compression format to transcode Red ETC1S textures
   * into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat ETC1S_R = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode Red-Green ETC1S
   * textures into, if one exists. Otherwise it will be decompressed into raw
   * pixels.
   */
  GpuCompressedPixelFormat ETC1S_RG = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode RGB ETC1S textures
   * into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat ETC1S_RGB = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode RGBA ETC1S textures
   * into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat ETC1S_RGBA = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode Red UASTC textures
   * into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat UASTC_R = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode Red-Green UASTC
   * textures into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat UASTC_RG = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode RGB UASTC textures
   * into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat UASTC_RGB = GpuCompressedPixelFormat::NONE;

  /**
   * @brief The gpu pixel compression format to transcode RGBA UASTC textures
   * into. If NONE, it will be decompressed into raw pixels.
   */
  GpuCompressedPixelFormat UASTC_RGBA = GpuCompressedPixelFormat::NONE;

  Ktx2TranscodeTargets() = default;

  /**
   * @brief Determine ideal transcode targets based on a list of supported gpu
   * compressed formats.
   *
   * @param supportedFormats The supported gpu compressed pixel formats.
   * @param preserveHighQuality Whether to preserve texture quality when
   * transcoding KTXv2 textures. If this is true, the texture may be fully
   * decompressed instead of picking a lossy target gpu compressed pixel format.
   */
  Ktx2TranscodeTargets(
      const SupportedGpuCompressedPixelFormats& supportedFormats,
      bool preserveHighQuality);
};

} // namespace CesiumGltf