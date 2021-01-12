#include "decodeDraco.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/KHR_draco_mesh_compression.h"
#include "CesiumGltf/Reader.h"
#include "CesiumGltf/Helpers.h"
#include <string>

#pragma warning(push)
#pragma warning(disable:4127)
#include <draco/core/decoder_buffer.h>
#include <draco/compression/decode.h>
#pragma warning(pop)


namespace {
    using namespace CesiumGltf;

    std::unique_ptr<draco::Mesh> decodeBufferViewToDracoMesh(
        ModelReaderResult& readModel,
        MeshPrimitive& /* primitive */,
        KHR_draco_mesh_compression& draco
    ) {
        Model& model = readModel.model.value();

        if (draco.bufferView < 0 || draco.bufferView >= model.bufferViews.size()) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "Draco bufferView index is invalid.";
            return nullptr;
        }

        BufferView& bufferView = model.bufferViews[draco.bufferView];

        if (bufferView.buffer < 0 || bufferView.buffer >= model.buffers.size()) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "Draco bufferView has an invalid buffer index.";
            return nullptr;
        }

        Buffer& buffer = model.buffers[bufferView.buffer];

        if (bufferView.byteOffset < 0 || bufferView.byteLength < 0 || bufferView.byteOffset + bufferView.byteLength > static_cast<int64_t>(buffer.cesium.data.size())) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "Draco bufferView extends beyond its buffer.";
            return nullptr;
        }

        gsl::span<const uint8_t> data(buffer.cesium.data.data() + bufferView.byteOffset, bufferView.byteLength);

        draco::DecoderBuffer decodeBuffer;
        decodeBuffer.Init(reinterpret_cast<const char*>(data.data()), data.size());

        draco::Decoder decoder;
        draco::Mesh mesh;
        draco::StatusOr<std::unique_ptr<draco::Mesh>> result = decoder.DecodeMeshFromBuffer(&decodeBuffer);
        if (!result.ok()) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "Draco decoding failed: ";
            readModel.warnings += result.status().error_msg_string();
            return nullptr;
        }

        return std::move(result).value();
    }

    template <typename TSource, typename TDestination>
    void copyData(const TSource* pSource, TDestination* pDestination, int64_t length) {
        std::transform(pSource, pSource + length, pDestination, [](auto x) { return static_cast<TDestination>(x); });
    }

    template <typename T>
    void copyData(const T* pSource, T* pDestination, int64_t length) {
        std::copy(pSource, pSource + length, pDestination);
    }

    void copyDecodedIndices(
        ModelReaderResult& readModel,
        MeshPrimitive& primitive,
        draco::Mesh* pMesh
    ) {
        Model& model = readModel.model.value();

        if (primitive.indices < 0) {
            return;
        }

        Accessor* pIndicesAccessor = getSafe(&model.accessors, primitive.indices);
        if (!pIndicesAccessor) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "Primitive indices accessor ID is invalid.";
            return;
        }

        if (pIndicesAccessor->count > pMesh->num_faces() * 3) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "There are fewer decoded Draco indices than are expected by the accessor.";

            pIndicesAccessor->count = pMesh->num_faces() * 3;
        }

        pIndicesAccessor->bufferView = static_cast<int32_t>(model.bufferViews.size());
        BufferView& indicesBufferView = model.bufferViews.emplace_back();

        indicesBufferView.buffer = static_cast<int32_t>(model.buffers.size());
        Buffer& indicesBuffer = model.buffers.emplace_back();

        int64_t indexBytes = computeByteSizeOfComponent(pIndicesAccessor->componentType);
        int64_t indicesBytes = pIndicesAccessor->count * indexBytes;
        
        indicesBuffer.cesium.data.resize(indicesBytes);
        indicesBuffer.byteLength = indicesBytes;
        indicesBufferView.byteLength = indicesBytes;
        indicesBufferView.byteStride = indexBytes;
        indicesBufferView.byteOffset = 0;
        indicesBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;
        pIndicesAccessor->type = Accessor::Type::SCALAR;

        static_assert(sizeof(draco::PointIndex) == sizeof(uint32_t));

        const uint32_t* pSourceIndices = reinterpret_cast<const uint32_t*>(&pMesh->face(draco::FaceIndex(0))[0]);

        switch (pIndicesAccessor->componentType) {
            case Accessor::ComponentType::BYTE:
                copyData(pSourceIndices, reinterpret_cast<int8_t*>(indicesBuffer.cesium.data.data()), pIndicesAccessor->count);
                break;
            case Accessor::ComponentType::UNSIGNED_BYTE:
                copyData(pSourceIndices, reinterpret_cast<uint8_t*>(indicesBuffer.cesium.data.data()), pIndicesAccessor->count);
                break;
            case Accessor::ComponentType::SHORT:
                copyData(pSourceIndices, reinterpret_cast<int16_t*>(indicesBuffer.cesium.data.data()), pIndicesAccessor->count);
                break;
            case Accessor::ComponentType::UNSIGNED_SHORT:
                copyData(pSourceIndices, reinterpret_cast<uint16_t*>(indicesBuffer.cesium.data.data()), pIndicesAccessor->count);
                break;
            case Accessor::ComponentType::UNSIGNED_INT:
                copyData(pSourceIndices, reinterpret_cast<uint32_t*>(indicesBuffer.cesium.data.data()), pIndicesAccessor->count);
                break;
            case Accessor::ComponentType::FLOAT:
                copyData(pSourceIndices, reinterpret_cast<float*>(indicesBuffer.cesium.data.data()), pIndicesAccessor->count);
                break;
        }
    }

    void copyDecodedAttribute(
        ModelReaderResult& readModel,
        MeshPrimitive& /* primitive */,
        Accessor* pAccessor,
        const draco::Mesh* pMesh,
        const draco::PointAttribute* pAttribute
    ) {
        Model& model = readModel.model.value();

        switch (pAttribute->data_type()) {
            case draco::DataType::DT_INT8:
                pAccessor->componentType = Accessor::ComponentType::BYTE;
                break;
            case draco::DataType::DT_UINT8:
                pAccessor->componentType = Accessor::ComponentType::UNSIGNED_BYTE;
                break;
            case draco::DataType::DT_INT16:
                pAccessor->componentType = Accessor::ComponentType::SHORT;
                break;
            case draco::DataType::DT_UINT16:
                pAccessor->componentType = Accessor::ComponentType::UNSIGNED_SHORT;
                break;
            case draco::DataType::DT_UINT32:
                pAccessor->componentType = Accessor::ComponentType::UNSIGNED_INT;
                break;
            case draco::DataType::DT_FLOAT32:
                pAccessor->componentType = Accessor::ComponentType::FLOAT;
                break;
            default:
                if (!readModel.warnings.empty()) readModel.warnings += "\n";
                readModel.warnings += "Unsupported Draco data type: " + std::to_string(pAttribute->data_type());
                return;
        }

        switch (pAttribute->num_components()) {
            case 1:
                pAccessor->type = Accessor::Type::SCALAR;
                break;
            case 2:
                pAccessor->type = Accessor::Type::VEC2;
                break;
            case 3:
                pAccessor->type = Accessor::Type::VEC3;
                break;
            case 4:
                // Could be a vector or a matrix. If the accessor is already one of those, keep it.
                // Otherwise go with VEC4.
                pAccessor->type =
                    pAccessor->type == Accessor::Type::VEC4 || pAccessor->type == Accessor::Type::MAT2
                        ? pAccessor->type
                        : Accessor::Type::VEC4;
                break;
            case 9:
                pAccessor->type = Accessor::Type::MAT3;
                break;
            case 16:
                pAccessor->type = Accessor::Type::MAT4;
                break;
            default:
                if (!readModel.warnings.empty()) readModel.warnings += "\n";
                readModel.warnings += "Unsupported number of components for Draco attribute: " + std::to_string(pAttribute->num_components());
                return;
        }

        if (pAccessor->count > pMesh->num_points()) {
            if (!readModel.warnings.empty()) readModel.warnings += "\n";
            readModel.warnings += "There are fewer decoded Draco indices than are expected by the accessor.";

            pAccessor->count = pMesh->num_points();
        }

        pAccessor->bufferView = static_cast<int32_t>(model.bufferViews.size());
        BufferView& bufferView = model.bufferViews.emplace_back();

        bufferView.buffer = static_cast<int32_t>(model.buffers.size());
        Buffer& buffer = model.buffers.emplace_back();

        int64_t stride = computeNumberOfComponents(pAccessor->type) * computeByteSizeOfComponent(pAccessor->componentType);
        int64_t sizeBytes = pAccessor->count * stride;

        buffer.cesium.data.resize(sizeBytes);
        buffer.byteLength = sizeBytes;
        bufferView.byteLength = sizeBytes;
        bufferView.byteStride = stride;
        bufferView.byteOffset = 0;
        pAccessor->byteOffset = 0;

        auto doCopy = [pMesh, pAttribute](auto pOut) {
            for (draco::PointIndex i(0); i < pMesh->num_points(); ++i) {
                draco::AttributeValueIndex valueIndex = pAttribute->mapped_index(i);
                pAttribute->ConvertValue(valueIndex, pOut);
                pOut += pAttribute->num_components();
            }
        };

        switch (pAccessor->componentType) {
            case Accessor::ComponentType::BYTE:
                doCopy(reinterpret_cast<int8_t*>(buffer.cesium.data.data()));
                break;
            case Accessor::ComponentType::UNSIGNED_BYTE:
                doCopy(reinterpret_cast<uint8_t*>(buffer.cesium.data.data()));
                break;
            case Accessor::ComponentType::SHORT:
                doCopy(reinterpret_cast<int16_t*>(buffer.cesium.data.data()));
                break;
            case Accessor::ComponentType::UNSIGNED_SHORT:
                doCopy(reinterpret_cast<uint16_t*>(buffer.cesium.data.data()));
                break;
            case Accessor::ComponentType::UNSIGNED_INT:
                doCopy(reinterpret_cast<uint32_t*>(buffer.cesium.data.data()));
                break;
            case Accessor::ComponentType::FLOAT:
                doCopy(reinterpret_cast<float*>(buffer.cesium.data.data()));
                break;
        }
    }

    void decodePrimitive(
        ModelReaderResult& readModel,
        MeshPrimitive& primitive,
        KHR_draco_mesh_compression& draco,
        std::unordered_map<int32_t, std::unique_ptr<draco::Mesh>>& bufferViewToMesh
    ) {
        Model& model = readModel.model.value();

        draco::Mesh* pMesh;

        auto cachedIt = bufferViewToMesh.find(draco.bufferView);
        if (cachedIt != bufferViewToMesh.end()) {
            pMesh = cachedIt->second.get();
        } else {
            std::unique_ptr<draco::Mesh> pNewMesh = decodeBufferViewToDracoMesh(readModel, primitive, draco);
            if (!pNewMesh) {
                return;
            }

            pMesh = pNewMesh.get();
            bufferViewToMesh.emplace(draco.bufferView, std::move(pNewMesh));
        }

        copyDecodedIndices(readModel, primitive, pMesh);

        for (const std::pair<std::string, int32_t>& attribute : draco.attributes) {
            auto primitiveAttrIt = primitive.attributes.find(attribute.first);
            if (primitiveAttrIt == primitive.attributes.end()) {
                // The primitive does not use this attribute. The KHR_draco_mesh_compression spec
                // says this shouldn't happen, so warn.
                if (!readModel.warnings.empty()) {
                    readModel.warnings += "\n";
                }
                readModel.warnings += "Draco extension has the " + attribute.first + " attribute, but the primitive does not have that attribute.";
                continue;
            }

            int32_t primitiveAttrIndex = primitiveAttrIt->second;
            Accessor* pAccessor = getSafe(&model.accessors, primitiveAttrIndex);
            if (!pAccessor) {
                if (!readModel.warnings.empty()) {
                    readModel.warnings += "\n";
                }
                readModel.warnings += "Primitive attribute's accessor index is invalid.";
                continue;
            }

            int32_t dracoAttrIndex = attribute.second;
            const draco::PointAttribute* pAttribute = pMesh->GetAttributeByUniqueId(dracoAttrIndex);
            if (pAttribute == nullptr) {
                if (!readModel.warnings.empty()) {
                    readModel.warnings += "\n";
                }
                readModel.warnings += "Draco attribute with unique ID " + std::to_string(dracoAttrIndex) + " does not exist.";
                continue;
            }

            copyDecodedAttribute(readModel, primitive, pAccessor, pMesh, pAttribute);
       }
    }
}

namespace CesiumGltf {

void decodeDraco(CesiumGltf::ModelReaderResult& readModel) {
    if (!readModel.model) {
        return;
    }

    Model& model = readModel.model.value();

    std::unordered_map<int32_t, std::unique_ptr<draco::Mesh>> bufferViewToMesh;

    for (Mesh& mesh : model.meshes) {
        for (MeshPrimitive& primitive : mesh.primitives) {
            KHR_draco_mesh_compression* pDraco = primitive.getExtension<KHR_draco_mesh_compression>();
            if (!pDraco) {
                continue;
            }

            decodePrimitive(readModel, primitive, *pDraco, bufferViewToMesh);
        }
    }
}

}