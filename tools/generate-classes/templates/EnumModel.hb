{{#if docs.length}}
/**
{{#each docs}}
* {{.}}
{{/each}}
*/
{{/if}}
enum class {{model}} {
  {{#each values}}
  {{modelMember}}{{~#unless @last}},{{/unless}}
  {{/each}}
};

