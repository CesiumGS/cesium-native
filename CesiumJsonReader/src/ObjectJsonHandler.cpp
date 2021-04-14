#include "CesiumJsonReader/ObjectJsonHandler.h"

using namespace CesiumJsonReader;

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

IJsonHandler* ObjectJsonHandler::EndSubObject() { return nullptr; }

void ObjectJsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  if (this->getCurrentKey()) {
    context.emplace_back(std::string(".") + this->getCurrentKey());
  }
  this->parent()->reportWarning(warning, std::move(context));
}

const char* ObjectJsonHandler::getCurrentKey() const {
  return this->_currentKey;
}

void ObjectJsonHandler::setCurrentKey(const char* key) {
  this->_currentKey = key;
}
