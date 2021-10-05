/////////////////////////////////
// Implementation of enum {{scopedReader}}
/////////////////////////////////


void {{scopedReader}}::reset(IJsonHandler* pParent, {{scopedModel}}* pEnum) {
  JsonHandler::reset(pParent);
  this->_pEnum = pEnum;
}

IJsonHandler* {{scopedReader}}::readString(const std::string_view& str) {
  using namespace std::string_literals;

  assert(this->_pEnum);

  {{#each values}}
  if ("{{name}}"s == str) {
    *this->_pEnum = {{../scopedModel}}::{{name}};
    return this->parent();
  }
  {{/each}}

  return nullptr;
}
