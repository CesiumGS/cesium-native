#include "ModelJsonHandler.h"
#include <cassert>
#include <string>

using namespace CesiumGltf;

void ModelJsonHandler::reset(JsonHandler* pParent, Model* pModel) {
    ObjectJsonHandler::reset(pParent);
    this->_pModel = pModel;
}

JsonHandler* ModelJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pModel);
    if ("accessors"s == str) return property(this->_accessors, this->_pModel->accessors);
    if ("meshes"s == str) return property(this->_meshes, this->_pModel->meshes);

    return this->ignore();
}
