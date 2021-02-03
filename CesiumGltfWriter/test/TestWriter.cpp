#include "catch2/catch.hpp"
#include "CesiumGltf/AccessorSparseIndices.h"
#include "CesiumGltf/Writer.h"
#include "CesiumGltf/Scene.h"
#include "CesiumGltf/Node.h"
#include "CesiumGltf/Mesh.h"
#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/BufferView.h"
#include <CesiumGltf/MeshPrimitive.h>
#include <gsl/span>
#include <string>

using namespace CesiumGltf;

Model generateTriangleModel() {
    Buffer buffer;
    buffer.uri = "data:application/octet-stream;base64,AAABAAIAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAA=";
    buffer.byteLength = 44;

    BufferView indicesBufferView;
    indicesBufferView.buffer = 0;
    indicesBufferView.byteOffset = 0;
    indicesBufferView.byteLength = 6;
    indicesBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

    BufferView vertexBufferView;
    vertexBufferView.buffer = 0;
    vertexBufferView.byteOffset = 8;
    vertexBufferView.byteLength = 36;
    vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

    Accessor indicesAccessor;
    indicesAccessor.bufferView = 0;
    indicesAccessor.byteOffset = 0;
    indicesAccessor.componentType = AccessorSpec::ComponentType::UNSIGNED_SHORT;
    indicesAccessor.count = 3;
    indicesAccessor.type = AccessorSpec::Type::SCALAR;
    indicesAccessor.max = std::vector<double> { 2.0 };
    indicesAccessor.min = std::vector<double> { 0.0 };

    Accessor vertexAccessor;
    vertexAccessor.bufferView = 1;
    vertexAccessor.byteOffset = 0;
    vertexAccessor.componentType = AccessorSpec::ComponentType::FLOAT;
    vertexAccessor.count = 3;
    vertexAccessor.type = AccessorSpec::Type::VEC3;
    vertexAccessor.max = std::vector<double> { 1.0, 1.0, 0.0 };
    vertexAccessor.min = std::vector<double> { 0.0, 0.0, 0.0 };

    Node node;
    node.mesh = 0;

    Scene scene;
    scene.nodes.emplace_back(0);

    MeshPrimitive trianglePrimitive;
    trianglePrimitive.attributes.emplace("POSITION", 1);
    trianglePrimitive.indices = 0;

    Mesh mesh;
    mesh.primitives.emplace_back(std::move(trianglePrimitive));

    Model triangleModel;

    triangleModel.scenes.emplace_back(std::move(scene));
    triangleModel.nodes.emplace_back(std::move(node));
    triangleModel.meshes.emplace_back(std::move(mesh));
    triangleModel.buffers.emplace_back(std::move(buffer));
    triangleModel.bufferViews.emplace_back(std::move(indicesBufferView));
    triangleModel.bufferViews.emplace_back(std::move(vertexBufferView));
    triangleModel.accessors.emplace_back(std::move(indicesAccessor));
    triangleModel.accessors.emplace_back(std::move(vertexAccessor));
    triangleModel.asset.version = "2.0";
    return triangleModel;
}

TEST_CASE("CesiumGltf::Writer") {
    SECTION("Basic triangle is serialized to embedded glTF 2.0") {
        const auto triangleModel = generateTriangleModel();  
        writeModelToByteArray(triangleModel);
    }
}