#include "ObjectJsonHandler.h"

using namespace CesiumGltf;

JsonHandler* ObjectJsonHandler::StartObject() {
    ++this->_depth;
    if (this->_depth > 1) {
        return this->StartSubObject();
    }
    return this;
}

JsonHandler* ObjectJsonHandler::EndObject(size_t memberCount) {
    --this->_depth;

    if (this->_depth > 0)
        return this->EndSubObject(memberCount);
    else
        return this->parent();
}

JsonHandler* ObjectJsonHandler::StartSubObject() {
    return nullptr;
}

JsonHandler* ObjectJsonHandler::EndSubObject(size_t /*memberCount*/) {
    return nullptr;
}

JsonHandler* ObjectJsonHandler::ignore() {
    this->_ignoreHandler.reset(this);
    return &this->_ignoreHandler;
}
