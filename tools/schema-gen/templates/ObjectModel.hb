{{#if description}}
/**
* @brief {{description}}
*/
{{/if}}
struct CODEGEN_API {{model}} : public {{modelBase.type}} {
  static inline constexpr const char* TypeName = "{{model}}";
  {{#if extension}}
  static inline constexpr const char* ExtensionName = "{{extension}}";
  {{/if}}

  {{#each subtypes}}
  {{> object}}
  {{/each}}

  {{#each enums}}
  {{> enum}}
  {{/each}}

  {{#each properties}}
  {{#if docs.length}}
  /**
  {{#each docs}}
  * {{.}}
  {{/each}}
  */
  {{/if}}
  {{modelMember}}

  {{/each}}
};

