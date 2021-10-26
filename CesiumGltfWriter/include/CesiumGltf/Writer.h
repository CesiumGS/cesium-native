#pragma once

#include "WriteGLTFCallback.h"
#include "WriteModelOptions.h"
#include "WriteModelResult.h"
#include "WriterLibrary.h"

#include <CesiumGltf/Model.h>

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
 * @returns A {@link CesiumGltf::WriteModelResult} containing a list of
 * errors, warnings, and the final generated std::vector<std::byte> of the
 * glTF asset (as a GLB or GLTF).
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
 * be set or a URIErroneouslyDefined error will be returned.
 * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * a contains base64 data uri and its `cesium.data` or `cesium.pixelData` vector
 * is non empty, a AmbiguiousDataSource error will be returned.
 * - If a {@link CesiumGltf::Buffer} contains a base64 data uri and its
 * byteLength is not set, a ByteLengthNotSet error will be returned.
 * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * contains an external file uri and its `cesium.data` or `cesium.pixelData`
 * vector is empty, a MissingDataSource error will be returned.
 * - If a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * contains an external file uri, it will be ignored (use
 * {@link CesiumGltf::writeModelAndExternalFiles} for external file
 * support).
 */

CESIUMGLTFWRITER_API WriteModelResult
writeModelAsEmbeddedBytes(const Model& model, const WriteModelOptions& options);

/**
 * @brief Write a glTF or glb asset and its associated external images and
 * buffers to multiple files via user provided callback.
 *
 * @returns A {@link CesiumGltf::WriteModelResult} containing a list of
 * errors, warnings, and the final generated std::vector<std::byte> of the
 * glTF asset (as a GLB or GLTF).
 *
 * @param model Final assembled glTF asset, ready for serialization.
 * @param options Options to use for exporting the asset.
 * @param filename Filename of the final glTF / GLB asset or an encountered
 * external {@link CesiumGltf::ImageCesium} / {@link CesiumGltf::BufferCesium}
 * during the serialization process
 * @param writeGLTFCallback Callback the user must implement. The serializer
 * will invoke this callback everytime it encounters
 * a {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}
 * with an external file uri, giving the callback a chance to write the asset to
 * disk. The callback will be called a final time with the provided filename
 * string when its time to serialize the final `glb` or `gltf` to disk.
 * @details Simialr to {@link CesiumGltf::writeModelAsEmbeddedBytes}, with the
 * key variation that a filename will be automatically generated
 * for your {@link CesiumGltf::Buffer} or {@link CesiumGltf::Image}  if no
 * uri is provided but `buffer.cesium.data` or `image.cesium.pixelData`
 * is non-empty.
 */

CESIUMGLTFWRITER_API WriteModelResult writeModelAndExternalFiles(
    const Model& model,
    const WriteModelOptions& options,
    std::string_view filename,
    const WriteGLTFCallback& writeGLTFCallback);
} // namespace CesiumGltf
