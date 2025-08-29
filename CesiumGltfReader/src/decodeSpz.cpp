#include "decodeSpz.h"

#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/MeshPrimitive.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/ExtensionKhrGaussianSplatting.h>
#include <CesiumGltf/ExtensionKhrGaussianSplattingCompressionSpz2.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/JsonValue.h>

#include <fmt/format.h>
#include <glm/fwd.hpp>
#include <load-spz.h>
#include <splat-types.h>

#include <cmath>
#include <cstdlib>

namespace CesiumGltfReader {
namespace {

const float SH_C0 = 0.282095f;

const std::string ALTERNATE_EXT_NAME1 = "KHR_spz_gaussian_splats_compression";
const std::string ALTERNATE_EXT_NAME2 =
    "KHR_gaussian_splatting_compression_spz";

std::unique_ptr<spz::GaussianCloud> decodeBufferViewToGaussianCloud(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& /* primitive */,
    const CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2& spz) {
  CESIUM_TRACE("CesiumGltfReader::decodeBufferViewToGaussianCloud");
  CESIUM_ASSERT(readGltf.model.has_value());
  CesiumGltf::Model& model = readGltf.model.value();

  CesiumGltf::BufferView* pBufferView =
      CesiumGltf::Model::getSafe(&model.bufferViews, spz.bufferView);
  if (!pBufferView) {
    readGltf.warnings.emplace_back("SPZ bufferView index is invalid.");
    return nullptr;
  }

  const CesiumGltf::BufferView& bufferView = *pBufferView;

  CesiumGltf::Buffer* pBuffer =
      CesiumGltf::Model::getSafe(&model.buffers, bufferView.buffer);
  if (!pBuffer) {
    readGltf.warnings.emplace_back(
        "SPZ bufferView has an invalid buffer index.");
    return nullptr;
  }

  CesiumGltf::Buffer& buffer = *pBuffer;

  if (bufferView.byteOffset < 0 || bufferView.byteLength < 0 ||
      bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
    readGltf.warnings.emplace_back("SPZ bufferView extends beyond its buffer.");
    return nullptr;
  }

  spz::GaussianCloud gaussians = spz::loadSpz(
      reinterpret_cast<uint8_t*>(
          buffer.cesium.data.data() + bufferView.byteOffset),
      static_cast<int32_t>(bufferView.byteLength),
      spz::UnpackOptions{spz::CoordinateSystem::UNSPECIFIED});

  return std::make_unique<spz::GaussianCloud>(std::move(gaussians));
}

CesiumGltf::Accessor* findAccessor(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& attributeName) {
  const std::unordered_map<const std::string, int32_t>::const_iterator
      attributeIt = primitive.attributes.find(attributeName);
  if (attributeIt == primitive.attributes.end()) {
    readGltf.warnings.emplace_back(
        "Failed to find " + attributeName +
        " attribute on KHR_gaussian_splatting_compression_spz_2 primitive");
    return nullptr;
  }

  CesiumGltf::Accessor* pAccessor = CesiumGltf::Model::getSafe(
      &readGltf.model->accessors,
      attributeIt->second);
  if (!pAccessor) {
    readGltf.warnings.emplace_back(
        "Failed to find accessor at index " +
        std::to_string(attributeIt->second));
    return nullptr;
  }

  return pAccessor;
}

void copyShCoeff(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& primitive,
    CesiumGltf::Buffer& buffer,
    spz::GaussianCloud* pGaussian,
    int degree,
    int coeffIndex) {
  size_t offset = 0;
  if (degree == 1) {
    offset = static_cast<size_t>(coeffIndex * 3);
  } else if (degree == 2) {
    offset = static_cast<size_t>(9 + coeffIndex * 3);
  } else if (degree == 3) {
    offset = static_cast<size_t>(24 + coeffIndex * 3);
  }

  size_t shTotal = 0;
  if (pGaussian->shDegree == 1) {
    shTotal = static_cast<size_t>(3 * 3);
  } else if (pGaussian->shDegree == 2) {
    shTotal = static_cast<size_t>(8 * 3);
  } else if (pGaussian->shDegree == 3) {
    shTotal = static_cast<size_t>(15 * 3);
  }

  CesiumGltf::Accessor* pAccessor = findAccessor(
      readGltf,
      primitive,
      fmt::format(
          "KHR_gaussian_splatting:SH_DEGREE_{}_COEF_{}",
          degree,
          coeffIndex));
  if (!pAccessor) {
    return;
  }

  pAccessor->bufferView =
      static_cast<int32_t>(readGltf.model->bufferViews.size());
  CesiumGltf::BufferView& bufferView =
      readGltf.model->bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(readGltf.model->buffers.size() - 1);
  bufferView.byteLength = static_cast<int64_t>(
      sizeof(float) * static_cast<size_t>(pGaussian->numPoints) * 3);

  size_t start = buffer.cesium.data.size();
  bufferView.byteOffset = static_cast<int64_t>(start);
  buffer.cesium.data.resize(start + static_cast<size_t>(bufferView.byteLength));
  for (size_t i = offset; i < pGaussian->sh.size(); i += shTotal) {
    memcpy(
        buffer.cesium.data.data() + start,
        pGaussian->sh.data() + i,
        sizeof(float) * 3);
    start += sizeof(float) * 3;
  }
}

void decodePrimitive(
    GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& primitive,
    CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2& spz) {
  CESIUM_TRACE("CesiumGltfReader::decodePrimitive");
  CESIUM_ASSERT(readGltf.model.has_value());

  // TODO: handle different accessor component types

  std::unique_ptr<spz::GaussianCloud> pGaussian =
      decodeBufferViewToGaussianCloud(readGltf, primitive, spz);
  if (!pGaussian) {
    return;
  }

  const size_t bufferLength =
      sizeof(float) * (pGaussian->positions.size() + pGaussian->scales.size() +
                       pGaussian->scales.size() + pGaussian->rotations.size() +
                       pGaussian->alphas.size() + pGaussian->colors.size() +
                       pGaussian->sh.size());

  CesiumGltf::Buffer& buffer = readGltf.model->buffers.emplace_back();
  buffer.byteLength = static_cast<int64_t>(bufferLength);
  buffer.cesium.data.reserve(bufferLength);

  // Position and rotation can be copied verbatim
  CesiumGltf::Accessor* pPosAccessor =
      findAccessor(readGltf, primitive, "POSITION");
  if (pPosAccessor) {
    pPosAccessor->bufferView =
        static_cast<int32_t>(readGltf.model->bufferViews.size());
    CesiumGltf::BufferView& bufferView =
        readGltf.model->bufferViews.emplace_back();
    bufferView.buffer =
        static_cast<int32_t>(readGltf.model->buffers.size() - 1);
    bufferView.byteLength =
        static_cast<int64_t>(sizeof(float) * pGaussian->positions.size());

    size_t start = buffer.cesium.data.size();
    bufferView.byteOffset = static_cast<int64_t>(start);
    buffer.cesium.data.resize(
        start + static_cast<size_t>(bufferView.byteLength));
    memcpy(
        buffer.cesium.data.data() + start,
        pGaussian->positions.data(),
        sizeof(float) * pGaussian->positions.size());
  }

  CesiumGltf::Accessor* pRotAccessor =
      findAccessor(readGltf, primitive, "KHR_gaussian_splatting:ROTATION");
  if (pRotAccessor) {
    pRotAccessor->bufferView =
        static_cast<int32_t>(readGltf.model->bufferViews.size());
    CesiumGltf::BufferView& bufferView =
        readGltf.model->bufferViews.emplace_back();
    bufferView.buffer =
        static_cast<int32_t>(readGltf.model->buffers.size() - 1);
    bufferView.byteLength =
        static_cast<int64_t>(sizeof(float) * pGaussian->rotations.size());

    size_t start = buffer.cesium.data.size();
    bufferView.byteOffset = static_cast<int64_t>(start);
    buffer.cesium.data.resize(
        start + static_cast<size_t>(bufferView.byteLength));
    memcpy(
        buffer.cesium.data.data() + start,
        pGaussian->rotations.data(),
        sizeof(float) * pGaussian->rotations.size());
  }

  // Color needs to be interleaved with alphas and have its values converted
  CesiumGltf::Accessor* pColorAccessor =
      findAccessor(readGltf, primitive, "COLOR_0");
  if (pColorAccessor) {
    pColorAccessor->bufferView =
        static_cast<int32_t>(readGltf.model->bufferViews.size());
    CesiumGltf::BufferView& bufferView =
        readGltf.model->bufferViews.emplace_back();
    bufferView.buffer =
        static_cast<int32_t>(readGltf.model->buffers.size() - 1);
    bufferView.byteLength = static_cast<int64_t>(
        (pGaussian->colors.size() + pGaussian->alphas.size()));

    size_t start = buffer.cesium.data.size();
    buffer.cesium.data.resize(
        start + static_cast<size_t>(bufferView.byteLength));
    bufferView.byteOffset = static_cast<int64_t>(start);
    for (size_t i = 0; i < pGaussian->alphas.size(); i++) {
      glm::fvec4 color(
          0.5 + pGaussian->colors[i * 3] * SH_C0,
          0.5 + pGaussian->colors[i * 3 + 1] * SH_C0,
          0.5 + pGaussian->colors[i * 3 + 2] * SH_C0,
          1.0 / (1.0 + exp(-pGaussian->alphas[i])));
      glm::u8vec4 coloru8(
          (uint8_t)(color.x * 0xff),
          (uint8_t)(color.y * 0xff),
          (uint8_t)(color.z * 0xff),
          (uint8_t)(color.w * 0xff));
      memcpy(
          buffer.cesium.data.data() + start + i * sizeof(glm::u8vec4),
          &coloru8,
          sizeof(glm::u8vec4));
    }
  }

  // Scale needs to be converted
  CesiumGltf::Accessor* pScaleAccessor =
      findAccessor(readGltf, primitive, "KHR_gaussian_splatting:SCALE");
  if (pScaleAccessor) {
    pScaleAccessor->bufferView =
        static_cast<int32_t>(readGltf.model->bufferViews.size());
    CesiumGltf::BufferView& bufferView =
        readGltf.model->bufferViews.emplace_back();
    bufferView.buffer =
        static_cast<int32_t>(readGltf.model->buffers.size() - 1);
    bufferView.byteLength =
        static_cast<int64_t>(sizeof(float) * pGaussian->scales.size());

    size_t start = buffer.cesium.data.size();
    buffer.cesium.data.resize(
        start + static_cast<size_t>(bufferView.byteLength));
    bufferView.byteOffset = static_cast<int64_t>(start);
    for (size_t i = 0; i < pGaussian->scales.size(); i++) {
      float scale = exp(pGaussian->scales[i]);
      memcpy(
          buffer.cesium.data.data() + start + i * sizeof(float),
          &scale,
          sizeof(float));
    }
  }

  if (pGaussian->shDegree > 0) {
    for (int i = 0; i < 3; i++) {
      copyShCoeff(readGltf, primitive, buffer, pGaussian.get(), 1, i);
    }
  }

  if (pGaussian->shDegree > 1) {
    for (int i = 0; i < 5; i++) {
      copyShCoeff(readGltf, primitive, buffer, pGaussian.get(), 2, i);
    }
  }

  if (pGaussian->shDegree > 2) {
    for (int i = 0; i < 7; i++) {
      copyShCoeff(readGltf, primitive, buffer, pGaussian.get(), 3, i);
    }
  }
}

CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2*
addExtensionFromJsonValue(
    const std::string& extName,
    CesiumGltfReader::GltfReaderResult& readGltf,
    CesiumGltf::ExtensionKhrGaussianSplatting& splatting,
    CesiumUtility::JsonValue* pKhrJson) {

  if (!pKhrJson->isObject()) {
    readGltf.errors.push_back(fmt::format("Invalid {} extension", extName));
    return nullptr;
  }
  const CesiumUtility::JsonValue::Object::const_iterator it =
      pKhrJson->getObject().find("bufferView");
  if (it == pKhrJson->getObject().end()) {
    readGltf.errors.push_back(
        fmt::format("No `bufferView` property found on {} extension", extName));
    return nullptr;
  }
  if (!it->second.isInt64() && !it->second.isUint64()) {
    readGltf.errors.push_back(fmt::format(
        "`bufferView` property on {} extension must be an integer value",
        extName));
    return nullptr;
  }

  CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2& ext =
      splatting.addExtension<
          CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2>();
  ext.bufferView = it->second.isInt64()
                       ? static_cast<int32_t>(it->second.getInt64())
                       : static_cast<int32_t>(it->second.getUint64());
  return &ext;
}

// Maps attribute names from older versions of the extension to the names from
// the current version of the extension.
void fixAttributeNames(CesiumGltf::MeshPrimitive& primitive) {
  std::vector<std::string> attributesToConvert;
  attributesToConvert.reserve(primitive.attributes.size());
  for (std::pair<const std::string, int32_t>& attribute :
       primitive.attributes) {
    if (attribute.first == "_SCALE" || attribute.first == "_ROTATION" ||
        attribute.first.starts_with("_SH_DEGREE_")) {
      attributesToConvert.push_back(attribute.first);
    }
  }

  for (const std::string& oldName : attributesToConvert) {
    const int32_t accessorIndex = primitive.attributes[oldName];
    primitive.attributes.erase(oldName);
    primitive.attributes.emplace(
        "KHR_gaussian_splatting:" + oldName.substr(1),
        accessorIndex);
  }
}

CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2*
getAndMaybeConvertSpzExtension(
    CesiumGltfReader::GltfReaderResult& readGltf,
    CesiumGltf::MeshPrimitive& primitive,
    CesiumGltf::ExtensionKhrGaussianSplatting& splatting) {

  // Check for the real thing.
  CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2* pSpz =
      splatting.getExtension<
          CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2>();
  if (pSpz) {
    return pSpz;
  }

  // Check for the old versions.
  CesiumUtility::JsonValue* pKhrSpz =
      primitive.getGenericExtension(ALTERNATE_EXT_NAME1);
  if (pKhrSpz != nullptr) {
    CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2* pResult =
        addExtensionFromJsonValue(
            ALTERNATE_EXT_NAME1,
            readGltf,
            splatting,
            pKhrSpz);
    primitive.extensions.erase(ALTERNATE_EXT_NAME1);
    return pResult;
  }

  CesiumUtility::JsonValue* pSpzNoVersion =
      splatting.getGenericExtension(ALTERNATE_EXT_NAME2);
  if (pSpzNoVersion != nullptr) {
    CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2* pResult =
        addExtensionFromJsonValue(
            ALTERNATE_EXT_NAME2,
            readGltf,
            splatting,
            pSpzNoVersion);
    primitive.extensions.erase(ALTERNATE_EXT_NAME2);
    return pResult;
  }

  return nullptr;
}
} // namespace

void decodeSpz(CesiumGltfReader::GltfReaderResult& readGltf) {
  CESIUM_TRACE("CesiumGltfReader::decodeSpz");
  if (!readGltf.model) {
    return;
  }

  CesiumGltf::Model& model = readGltf.model.value();

  for (CesiumGltf::Mesh& mesh : model.meshes) {
    for (CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
      // KHR_spz_gaussian_splats_compression has no KHR_gaussian_splatting
      // extension attached Just throw one on there to make this easier
      if (primitive.extensions.contains(ALTERNATE_EXT_NAME1)) {
        primitive.addExtension<CesiumGltf::ExtensionKhrGaussianSplatting>();
      }

      CesiumGltf::ExtensionKhrGaussianSplatting* pSplat =
          primitive.getExtension<CesiumGltf::ExtensionKhrGaussianSplatting>();
      if (!pSplat) {
        continue;
      }

      CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2* pSpz =
          getAndMaybeConvertSpzExtension(readGltf, primitive, *pSplat);
      if (!pSpz) {
        continue;
      }

      fixAttributeNames(primitive);
      decodePrimitive(readGltf, primitive, *pSpz);

      // Remove the SPZ extension as it no longer applies.
      pSplat->extensions.erase(
          CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2::
              ExtensionName);
    }
  }

  model.removeExtensionRequired(
      CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2::ExtensionName);
  model.removeExtensionRequired(ALTERNATE_EXT_NAME1);
  model.removeExtensionRequired(ALTERNATE_EXT_NAME2);
}

bool hasSpzExtension(GltfReaderResult& readGltf) {
  if (readGltf.model->isExtensionUsed(
          CesiumGltf::ExtensionKhrGaussianSplattingCompressionSpz2::
              ExtensionName)) {
    return true;
  }

  if (readGltf.model->isExtensionUsed(ALTERNATE_EXT_NAME1)) {
    return true;
  }

  if (readGltf.model->isExtensionUsed(ALTERNATE_EXT_NAME2)) {
    return true;
  }

  return false;
}
} // namespace CesiumGltfReader
