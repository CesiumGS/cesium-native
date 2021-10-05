{{#each subtypes}}
{{> object}}
{{/each}}

{{#each enums}}
{{> enum}}
{{/each}}

/////////////////////////////////////////
// Writer for {{scopedModel}}
/////////////////////////////////////////

namespace {
  void writeJson(const {{scopedModel}}& obj, JsonWriter& jsonWriter, const ExtensionWriterContext& context) {
    jsonWriter.StartObject();

    {{#each properties}}

    {{~#if isAdditional}}
    writeJson(obj.{{name}}, jsonWriter, context);
    {{else}}
    {{#if required}}
    jsonWriter.Key("{{name}}");
    writeJson(obj.{{name}}, jsonWriter, context);
    {{else}}
    if (obj.{{name}}.has_value()) {
      jsonWriter.Key("{{name}}");
      writeJson(obj.{{name}}, jsonWriter, context);
    }
    {{/if}}
    {{/if}}

    {{/each}}

    if (!obj.extensions.empty()) {
      jsonWriter.Key("extensions");
      writeJsonExtensions(obj, jsonWriter, context);
    }

    jsonWriter.EndObject();
  }
}

void {{baseName}}Writer::write(const {{scopedModel}}& obj, JsonWriter& jsonWriter, const ExtensionWriterContext& context) {
  writeJson(obj, jsonWriter, context);
}
