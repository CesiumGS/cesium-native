{{#each subtypes}}
{{> object}}
{{/each}}

{{#each enums}}
{{> enum}}
{{/each}}

struct {{baseName}}Writer {
  using ValueType = {{scopedModel}};

  {{#if extension}}
  static inline constexpr const char* ExtensionName = "{{extension}}";
  {{/if}}

  static void write(const {{scopedModel}}& obj, CesiumJsonWriter::JsonWriter& jsonWriter, const CesiumJsonWriter::ExtensionWriterContext& context);
};
