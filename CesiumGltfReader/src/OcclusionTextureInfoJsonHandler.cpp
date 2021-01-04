#include "OcclusionTextureInfoJsonHandler.h"
#include "CesiumGltf/OcclusionTextureInfo.h"
#include <string>
#include <cassert>

using namespace CesiumGltf;

void OcclusionTextureInfoJsonHandler::reset(JsonHandler* pParent, OcclusionTextureInfo* pOcclusionTextureInfo) {
    JsonHandler::reset(pParent);
    this->_pOcclusionTextureInfo = pOcclusionTextureInfo;
}

JsonHandler* OcclusionTextureInfoJsonHandler::Key(const char* str, size_t length, bool copy) {
    using namespace std::string_literals;

    assert(this->_pOcclusionTextureInfo);

    if ("strength"s == str) return property(this->_strength, this->_pOcclusionTextureInfo->strength);

    return TextureInfoJsonHandler::Key(str, length, copy);
}
