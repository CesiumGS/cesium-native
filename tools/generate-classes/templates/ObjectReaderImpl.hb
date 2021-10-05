/////////////////////////////////////////
// Implementation of {{scopedReader}}
/////////////////////////////////////////

{{scopedReader}}::{{reader}}({{readerBase.params}}) : {{readerBase.type}}({{readerBase.args}})
  {{#each properties}}
  , _{{name}}({{readerArgs}})
  {{/each}}
{}

void {{scopedReader}}::reset(IJsonHandler* pParentHandler, {{scopedModel}}* pObject) {
  {{readerBase.type}}::reset(pParentHandler, pObject);
  this->_pObject = pObject;
  {{#if hasAdditional}}
  this->_additionalProperties.reset(pParentHandler, &pObject->additionalProperties);
  {{/if}}
}

IJsonHandler* {{scopedReader}}::readObjectKey(const std::string_view& str) {
  assert(this->_pObject);
  return this->readObjectKey{{baseName}}({{scopedModel}}::TypeName, str, *this->_pObject);
}

{{#if extension}}
void {{scopedReader}}::reset(IJsonHandler* pParentHandler, ExtensibleObject& o, const std::string_view& extensionName) {
  std::any& value = o.extensions.emplace(extensionName, {{scopedModel}}()).first->second;
  this->reset(pParentHandler, &std::any_cast<{{scopedModel}}&>(value));
}
{{/if~}}

IJsonHandler* {{scopedReader}}::readObjectKey{{baseName}}([[maybe_unused]] const std::string& objectType, const std::string_view& str, [[maybe_unused]]{{scopedModel}}& o) {
  using namespace std::string_literals;

  {{~#each properties}}
  {{#unless isAdditional}}
  
  if ("{{name}}"s == str)
    return property("{{name}}", this->_{{name}}, o.{{name}});

  {{/unless}}
  {{~/each}}

  {{#if hasAdditional}}
  return this->_additionalProperties.readObjectKey(str);
  {{else}}
  return this->readObjectKey{{modelBase.baseName}}(objectType, str, *this->_pObject);
  {{/if}}
}

{{#each subtypes}}
{{> object}}
{{/each}}

{{#each enums}}
{{> enum}}
{{/each}}
