#include "ObjectJsonHandler.h"

using namespace CesiumGltf;

IJsonHandler* ObjectJsonHandler::StartObject() {
  ++this->_depth;
  if (this->_depth > 1) {
    return this->StartSubObject();
  }
  return this;
}

IJsonHandler* ObjectJsonHandler::EndObject(size_t memberCount) {
  this->_currentKey = nullptr;

  --this->_depth;

  if (this->_depth > 0)
    return this->EndSubObject(memberCount);

  return this->parent();
}

IJsonHandler* ObjectJsonHandler::StartSubObject() { return nullptr; }

IJsonHandler* ObjectJsonHandler::EndSubObject(size_t /*memberCount*/) {
  return nullptr;
}

const char* ObjectJsonHandler::getCurrentKey() const {
  return this->_currentKey;
}
