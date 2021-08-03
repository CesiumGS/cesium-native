/////////////////////////////////////////
// Writer for {{scopedModel}}
/////////////////////////////////////////

namespace {
  void writeJson(const {{scopedModel}}& obj, JsonWriter& jsonWriter, const ExtensionWriterContext& context) {
    using namespace std::string_literals;

    switch (obj) {
      {{#each values}}
      case {{../scopedModel}}::{{name}}:
        writeJson({{value}}, jsonWriter, context);
        break;
      {{/each}}
      default:
        jsonWriter.Null();
    }
  }
}

void {{baseName}}Writer::write(const {{scopedModel}}& obj, JsonWriter& jsonWriter, const ExtensionWriterContext& context) {
  writeJson(obj, jsonWriter, context);
}
