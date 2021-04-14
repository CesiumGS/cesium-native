#include "CesiumJsonReader/ObjectJsonHandler.h"

using namespace CesiumJsonReader;

IJsonReader* ObjectJsonHandler::readObjectStart() {
  ++this->_depth;
  if (this->_depth > 1) {
    return this->StartSubObject();
  }
  return this;
}

IJsonReader* ObjectJsonHandler::readObjectEnd() {
  this->_currentKey = nullptr;

  --this->_depth;

  if (this->_depth > 0)
    return this->EndSubObject();
  else
    return this->parent();
}

IJsonReader* ObjectJsonHandler::StartSubObject() { return nullptr; }

IJsonReader* ObjectJsonHandler::EndSubObject() { return nullptr; }

const char* ObjectJsonHandler::getCurrentKey() const {
  return this->_currentKey;
}

void ObjectJsonHandler::setCurrentKey(const char* key) {
  this->_currentKey = key;
}
