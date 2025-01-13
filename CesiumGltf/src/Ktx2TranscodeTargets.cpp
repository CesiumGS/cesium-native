
#include <CesiumGltf/Ktx2TranscodeTargets.h>

using namespace CesiumGltf;

Ktx2TranscodeTargets::Ktx2TranscodeTargets(
    const SupportedGpuCompressedPixelFormats& supportedFormats,
    bool preserveHighQuality) {

  // Attempts to determine ideal transcode target formats using the logic here:
  // https://github.com/KhronosGroup/3D-Formats-Guidelines/blob/main/KTXDeveloperGuide.md

  // Find transcode target for ETC1S RGBA
  if (supportedFormats.ETC2_RGBA) {
    this->ETC1S_RGBA = GpuCompressedPixelFormat::ETC2_RGBA;
    // } else if (suportedFormats.ETC1_RGB) {
    // It is recommended to transcode into two separate etc1_rgb images here,
    // but we'll just skip the hassle now and consider bc7 instead.
  } else if (supportedFormats.BC7_RGBA) {
    this->ETC1S_RGBA = GpuCompressedPixelFormat::BC7_RGBA;
  } else if (supportedFormats.BC3_RGBA) {
    this->ETC1S_RGBA = GpuCompressedPixelFormat::BC3_RGBA;
  } else if (supportedFormats.PVRTC1_4_RGBA) {
    this->ETC1S_RGBA = GpuCompressedPixelFormat::PVRTC1_4_RGBA;
  }

  // Find the transcode target for ETC1S RGB
  if (supportedFormats.ETC1_RGB) {
    this->ETC1S_RGB = GpuCompressedPixelFormat::ETC1_RGB;
  } else if (supportedFormats.BC7_RGBA) {
    this->ETC1S_RGB = GpuCompressedPixelFormat::BC7_RGBA;
  } else if (supportedFormats.BC1_RGB) {
    this->ETC1S_RGB = GpuCompressedPixelFormat::BC1_RGB;
  } else if (supportedFormats.PVRTC1_4_RGB) {
    this->ETC1S_RGB = GpuCompressedPixelFormat::PVRTC1_4_RGB;
  }

  // Find the transcode target for ETC1S Red-Green
  if (supportedFormats.ETC2_EAC_RG11) {
    this->ETC1S_RG = GpuCompressedPixelFormat::ETC2_EAC_RG11;
    // } else if (supportedFormats.ETC1_RGB) {
    // It is recommended to transcode into two separate etc1_rgb images here,
    // but we'll just skip the hassle now and consider bc5 instead.
  } else if (supportedFormats.BC5_RG) {
    this->ETC1S_RG = GpuCompressedPixelFormat::BC5_RG;
  }

  // Find the transcode target for ETC1S Red
  if (supportedFormats.ETC1_RGB) {
    this->ETC1S_R = GpuCompressedPixelFormat::ETC1_RGB;
  } else if (supportedFormats.BC4_R) {
    this->ETC1S_R = GpuCompressedPixelFormat::BC4_R;
  } else if (supportedFormats.BC1_RGB) {
    this->ETC1S_R = GpuCompressedPixelFormat::BC1_RGB;
  } else if (supportedFormats.PVRTC1_4_RGB) {
    this->ETC1S_R = GpuCompressedPixelFormat::PVRTC1_4_RGB;
  }

  // Find the transcode target for UASTC RGBA
  if (supportedFormats.ASTC_4x4_RGBA) {
    this->UASTC_RGBA = GpuCompressedPixelFormat::ASTC_4x4_RGBA;
  } else if (supportedFormats.BC7_RGBA) {
    this->UASTC_RGBA = GpuCompressedPixelFormat::BC7_RGBA;
  } else if (!preserveHighQuality) {
    if (supportedFormats.ETC2_RGBA) {
      this->UASTC_RGBA = GpuCompressedPixelFormat::ETC2_RGBA;
      // } else if (supportedFormats.ETC1_RGB) {
      // It is recommended to transcode into two separate etc1_rgb images here,
      // but we'll just skip the hassle now and consider bc3 instead.
    } else if (supportedFormats.BC3_RGBA) {
      this->UASTC_RGBA = GpuCompressedPixelFormat::BC3_RGBA;
    } else if (supportedFormats.PVRTC1_4_RGBA) {
      this->UASTC_RGBA = GpuCompressedPixelFormat::PVRTC1_4_RGBA;
    }
  }
  // TODO: else { decodeToQuantized ? }

  // Find the transcode target for UASTC RGB
  if (supportedFormats.ASTC_4x4_RGBA) {
    this->UASTC_RGB = GpuCompressedPixelFormat::ASTC_4x4_RGBA;
  } else if (supportedFormats.BC7_RGBA) {
    this->UASTC_RGB = GpuCompressedPixelFormat::BC7_RGBA;
  } else if (!preserveHighQuality) {
    if (supportedFormats.ETC1_RGB) {
      this->UASTC_RGB = GpuCompressedPixelFormat::ETC1_RGB;
    } else if (supportedFormats.BC1_RGB) {
      this->UASTC_RGB = GpuCompressedPixelFormat::BC1_RGB;
    } else if (supportedFormats.PVRTC1_4_RGB) {
      this->UASTC_RGB = GpuCompressedPixelFormat::PVRTC1_4_RGB;
    }
  }
  // TODO: else { decodeToQuantized ? }

  // Find the transcode target for UASTC Red-Green
  if (supportedFormats.ASTC_4x4_RGBA) {
    this->UASTC_RG = GpuCompressedPixelFormat::ASTC_4x4_RGBA;
  } else if (supportedFormats.BC7_RGBA) {
    this->UASTC_RG = GpuCompressedPixelFormat::BC7_RGBA;
  } else if (!preserveHighQuality) {
    if (supportedFormats.ETC2_EAC_RG11) {
      this->UASTC_RG = GpuCompressedPixelFormat::ETC2_EAC_RG11;
    } else if (supportedFormats.BC5_RG) {
      this->UASTC_RG = GpuCompressedPixelFormat::BC5_RG;
    }
  }
  // TODO: else { decode to RG8 or RGR565 }

  // Find the transcode target for UASTC Red
  if (supportedFormats.ASTC_4x4_RGBA) {
    this->UASTC_R = GpuCompressedPixelFormat::ASTC_4x4_RGBA;
  } else if (supportedFormats.BC7_RGBA) {
    this->UASTC_R = GpuCompressedPixelFormat::BC7_RGBA;
  } else if (!preserveHighQuality) {
    if (supportedFormats.ETC2_EAC_R11) {
      this->UASTC_R = GpuCompressedPixelFormat::ETC2_EAC_R11;
    } else if (supportedFormats.BC4_R) {
      this->UASTC_R = GpuCompressedPixelFormat::BC4_R;
    }
  }
  // TODO: else { decode to R8 or RGR565 }

  // If any of the transmission formats stil has std::nullopt as it's transcode
  // target, then it will be fully decompressed into RGBA8 pixels.
}