#include "NormalTextureInfoJsonHandler.h"
#include "CesiumGltf/NormalTextureInfo.h"
#include <string>
#include <cassert>

using namespace CesiumGltf;

void NormalTextureInfoJsonHandler::reset(JsonHandler* pParent, NormalTextureInfo* pNormalTextureInfo) {
    JsonHandler::reset(pParent);
    this->_pNormalTextureInfo = pNormalTextureInfo;
}

JsonHandler* NormalTextureInfoJsonHandler::Key(const char* str, size_t length, bool copy) {
    using namespace std::string_literals;

    assert(this->_pNormalTextureInfo);

    if ("scale"s == str) return property(this->_scale, this->_pNormalTextureInfo->scale);

    return TextureInfoJsonHandler::Key(str, length, copy);
}
