#include "MeshJsonHandler.h"
#include <string>
#include <cassert>

using namespace CesiumGltf;

void MeshJsonHandler::reset(JsonHandler* pParent, Mesh* pMesh) {
    JsonHandler::reset(pParent);
    this->_pMesh = pMesh;
}

JsonHandler* MeshJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pMesh);
    if ("primitives"s == str) return property(this->_primitives, this->_pMesh->primitives);
    if ("weights"s == str) return property(this->_weights, this->_pMesh->weights);

    return this->ignore();
}
