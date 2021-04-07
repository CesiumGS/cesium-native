#include "ObjectJsonHandler.h"

using namespace CesiumGltf;

IJsonHandler* ObjectJsonHandler::readObjectStart() {
  ++this->_depth;
  if (this->_depth > 1) {
    return this->StartSubObject();
  }
  return this;
}

IJsonHandler* ObjectJsonHandler::readObjectEnd() {
  this->_currentKey = nullptr;

  --this->_depth;

  if (this->_depth > 0)
    return this->EndSubObject();
  else
    return this->parent();
}

IJsonHandler* ObjectJsonHandler::StartSubObject() { return nullptr; }

IJsonHandler* ObjectJsonHandler::EndSubObject() {
  return nullptr;
}

const char* ObjectJsonHandler::getCurrentKey() const {
  return this->_currentKey;
}

void ObjectJsonHandler::setCurrentKey(const char* key) {
  this->_currentKey = key;
}
