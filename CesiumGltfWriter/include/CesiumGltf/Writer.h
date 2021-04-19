#pragma once
#include "CesiumGltf/Model.h"
#include "WriteGLTFCallback.h"
#include "WriteOptions.h"
#include "WriterLibrary.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace CesiumGltf {

/**
 * @brief Write a glTF or glb asset to a byte vector.
 *
 * @param model Final assembled glTF asset, ready for serialization.
 * @param options Options to use for exporting the asset.
 *
 * @details Serializes the provided model object into a byte vector using the
 * provided flags to convert. There are a few special scenarios with
 * serializing {@link CesiumGltf::Buffer} and {@link CesiumGltf::Image}
 * objects:
 * - If {@link GltfExportType::GLB} is specified, `model.buffers[0].cesium.data`
 * will be used as the single binary data storage GLB chunk, so it's the callers
 * responsibility to place all their binary data in
 * `model.buffers[0].cesium.data` if they want it to be serialized to the GLB.
 * - If {@link GltfExportType::GLB} is specified, `model.buffers[0].uri` CANNOT
 * be set or a {@link CesiumGltf::URIErroneouslyDefined} exception will be
 * thrown.
 * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * a contains base64 data uri and its `cesium.data` or `cesium.pixelData` vector
 * is non empty, a {@link CesiumGltf::AmbiguiousDataSource} exception
 * will be thrown.
 * - If a {@link CesiumGltf::Buffer} contains a base64 data uri and its
 * byteLength is not set, a {@link CesiumGltf::ByteLengthNotSet}
 * exception will be thrown.
 * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * contains an external file uri and its `cesium.data` or `cesium.pixelData`
 * vector is empty, a {@link CesiumGltf::MissingDataSource} exception
 * will be thrown.
 * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * contains an external file uri, it will be ignored (use {@link
 * CesiumGltf::writeModelAndExternalFiles} for external file support).
 */

CESIUMGLTFWRITER_API std::vector<std::byte>
writeModelAsEmbeddedBytes(const Model& model, const WriteOptions& options);

/**
 * @brief Write a glTF or glb asset and its associated external images and
 buffers to
 * multiple files via user provided callback.

 * @param model Final assembled glTF asset, ready for serialization.
 * @param flags Bitset flags used to control writing glTF serialization
 behavior.
 * @param filename Filename of the final glTF / GLB asset or an encountered
 external
 * {@link CesiumGltf::ImageCesium} / {@link CesiumGltf::BufferCesium} during the
 serialization process
 * @param writeGLTFCallback Callback the user must implement. The serializer
 will invoke this callback everytime it encounters
 a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * with an external file uri, giving the callback a chance to write the asset to
 disk. The callback will be called a final time with the provided filename
 string when its time to serialize the final `glb` or `gltf` to disk.
 * @details Simialr to {@link CesiumGltf::writeModelAsEmbeddedBytes}, with the
 * key variation that a filename will be automatically generated
 for your {@link CesiumGltf::Buffer}
 * or {@link CesiumGltf::Image}  if no uri is provided but `buffer.cesium.data`
 or `image.cesium.pixelData` is non-empty.
 */

CESIUMGLTFWRITER_API void writeModelAndExternalFiles(
    const Model& model,
    const WriteOptions& options,
    std::string_view filename,
    WriteGLTFCallback writeGLTFCallback);
} // namespace CesiumGltf
