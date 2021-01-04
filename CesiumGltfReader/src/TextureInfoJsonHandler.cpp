#include "TextureInfoJsonHandler.h"
#include "CesiumGltf/TextureInfo.h"
#include <string>
#include <cassert>

using namespace CesiumGltf;

void TextureInfoJsonHandler::reset(JsonHandler* pParent, TextureInfo* pTextureInfo) {
    JsonHandler::reset(pParent);
    this->_pTextureInfo = pTextureInfo;
}

JsonHandler* TextureInfoJsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
    using namespace std::string_literals;

    assert(this->_pTextureInfo);

    if ("index"s == str) return property(this->_index, this->_pTextureInfo->index);
    if ("texCoord"s == str) return property(this->_texCoord, this->_pTextureInfo->texCoord);

    return this->ExtensibleObjectKey(str, *this->_pTextureInfo);
}
