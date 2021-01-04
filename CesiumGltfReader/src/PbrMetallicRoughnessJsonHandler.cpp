#include "PbrMetallicRoughnessJsonHandler.h"
#include "CesiumGltf/PbrMetallicRoughness.h"
#include <string>
#include <cassert>

using namespace CesiumGltf;

void PbrMetallicRoughnessJsonHandler::reset(JsonHandler* pParent, PbrMetallicRoughness* pPbr) {
    JsonHandler::reset(pParent);
    this->_pPbr = pPbr;
}

JsonHandler* PbrMetallicRoughnessJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pPbr);

    if ("baseColorFactor"s == str) return property(this->_baseColorFactor, this->_pPbr->baseColorFactor);
    if ("baseColorTexture"s == str) return property(this->_baseColorTexture, this->_pPbr->baseColorTexture);
    if ("metallicFactor"s == str) return property(this->_metallicFactor, this->_pPbr->metallicFactor);
    if ("roughnessFactor"s == str) return property(this->_roughnessFactor, this->_pPbr->roughnessFactor);
    if ("metallicRoughnessTexture"s == str) return property(this->_metallicRoughnessTexture, this->_pPbr->metallicRoughnessTexture);

    return this->ExtensibleObjectKey(str, *this->_pPbr);
}
