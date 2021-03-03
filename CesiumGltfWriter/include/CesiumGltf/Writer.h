#pragma once
#include "CesiumGltf/Model.h"
#include "WriteFlags.h"
#include "WriterLibrary.h"
#include "WriteGLTFCallback.h"
#include <cstdint>
#include <vector>
#include <string_view>

namespace CesiumGltf {
    /**
     * @brief Write a glTF or binary glTF (GLB) to one or more files.
     * 
     * @param filename Filename of final glTF or GLB asset, won't be manipulated in
     *                 any way, so it's the callers responsibility to add the necessary 
     *                 extension, will be passed to `writeGLTFCallback` when it's time
     *                 to serialize the final glTF asset.
     * @param model    Final assembled glTF asset, ready for serialization
     * @param writeGLTFCallback Invoked when it's time to serialize glTF assets to disk, it's
     *                          the callers responsibility to implement the `writeGLTFCallback` 
     *                          to serialize assets to disk in the correct location.
     * @param options Bitset flags used to control writing glTF serialization behavior.
     */

    CESIUMGLTFWRITER_API std::vector<std::uint8_t> writeModelAsEmbeddedBytes(
        const Model& model,
        WriteFlags flags
    );

    CESIUMGLTFWRITER_API void writeModelAndExternalFiles(
        const Model& model,
        WriteFlags flags,
        std::string_view filename,
        WriteGLTFCallback writeGLTFCallback
    );
}