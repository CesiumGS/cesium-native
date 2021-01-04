#include "MaterialJsonHandler.h"
#include "CesiumGltf/Material.h"
#include <cassert>
#include <string>

using namespace CesiumGltf;

void MaterialJsonHandler::reset(JsonHandler* pParent, Material* pMaterial) {
    ObjectJsonHandler::reset(pParent);
    this->_pMaterial = pMaterial;
}

JsonHandler* MaterialJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pMaterial);

    if ("pbrMetallicRoughness"s == str) return property(this->_pbrMetallicRoughness, this->_pMaterial->pbrMetallicRoughness);
    if ("emissiveTexture"s == str) return property(this->_emissiveTexture, this->_pMaterial->emissiveTexture);
    if ("emissiveFactor"s == str) return property(this->_emissiveFactor, this->_pMaterial->emissiveFactor);
    if ("alphaMode"s == str) return property(this->_alphaMode, this->_pMaterial->alphaMode);
    if ("alphaCutoff"s == str) return property(this->_alphaCutoff, this->_pMaterial->alphaCutoff);
    if ("doubleSided"s == str) return property(this->_doubleSided, this->_pMaterial->doubleSided);

    return this->NamedObjectKey(str, *this->_pMaterial);
}
