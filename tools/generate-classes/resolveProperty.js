const nameUtils = require("./nameUtils");
const lodash = require('lodash');

function getBaseProperty(name, details) {
  const fullDoc =
    details.gltf_detailedDescription &&
    details.gltf_detailedDescription.indexOf(
      details.description
    ) === 0
      ? details.gltf_detailedDescription
          .substr(details.description.length)
          .trim()
      : details.gltf_detailedDescription;

  const briefDocLines = details.description ? [`@brief ${details.description}`] : [];
  const fullDocLines = fullDoc ? fullDoc.split("\n") : [];

  return {
    docs: briefDocLines.concat(fullDocLines),
    typeName: nameUtils.makeTypeName(name),
    model: {
      type: nameUtils.makeTypeName(name),
      headers: []
    },
    reader: {
      type: nameUtils.makeTypeName(name, "JsonHandler"),
      headers: []
    },
  };
}

function topoSort(graph) {
  const tmpOrder = Object.keys(graph).filter((key) => graph[key].length === 0);
  const order = [];

  while (tmpOrder.length > 0) {
    const n = tmpOrder.pop();
    order.push(n);

    Object.keys(graph).forEach((m) => {
      const edges = graph[m];
      const removed = edges.filter((k) => k !== n);
      const found = removed.length !== edges.length;
      graph[m] = removed;

      if (found && removed.length === 0) {
        tmpOrder.push(m);
      }
    });
  }

  return order;
}

class Context {
  #scope;
  #schemaStack;
  #schemaCache;
  #resolveCache;
  #overrides;

  constructor(schemaCache, overrides) {
    this.#schemaStack = [];
    this.#scope = [];
    this.#schemaCache = schemaCache;
    this.#resolveCache = {};
    this.#overrides = overrides || {};
  }

  resolve(ref) {
    const existing = this.#resolveCache[ref];
    if (existing) {
      return existing;
    } else {
      const parts = ref.split("#");
      if (parts[0]) {
        this.#schemaStack.push(parts[0]);
      }

      const schemaName = this.#schemaStack[this.#schemaStack.length - 1];
      if (!schemaName) {
        console.warn("no schema currently being resolved");
        return undefined;
      }

      const schema = this.#schemaCache.load(schemaName, parts[1]);
      if (!schema) {
        console.warn("failed to load schema", schemaName);
        this.#schemaStack.pop();
        return undefined;
      }

      const override = this.#overrides[schemaName] || {};
      const result = this.create(schema, schemaName, this);
      if (!result) {
        console.warn("failed to resolve schema", schemaName);
        this.#schemaStack.pop();
        return undefined;
      }

      this.#resolveCache[ref] = result;
      this.#schemaStack.pop();
      return result;
    }
  }

  #resolveEnum(schema, name) {
    if (schema.type !== "string") {
      console.warn("non-string non-glTF-style enums not supported");
      return undefined;
    }

    return SchemaEnum.fromEnum(schema, name, this);
  }

  // detect if the anyOf is a glTF-style enum or an actual variant type (not supported)
  #resolveAnyOf(details, name) {
    const typeItem = details.anyOf
      .filter((item) => item.type !== undefined && item.enum === undefined)
      .pop();
    if (typeItem !== undefined) {
      const itemInfo = details.anyOf.filter((item) => item.enum !== undefined);
      return SchemaEnum.fromAnyOf(typeItem, itemInfo, name, this);
    } else {
      console.warn(`heterogeneous variants not supported`);
      return undefined;
    }
  }

  #createInScope(schema, name) {
    if (schema.allOf && schema.allOf.length == 1 && (schema.properties === undefined))
      return this.#createInScope(schema.allOf[0], name);
    if (schema.$ref)
      return new SchemaRef(schema.$ref, this);
    if (schema.anyOf)
      return this.#resolveAnyOf(schema, name);
    if (schema.enum)
      return this.#resolveEnum(schema, name);
    if (schema.type === "string")
      return new SchemaString(schema);
    if (schema.type === "integer")
      return new SchemaInteger(schema);
    if (schema.type === "number")
      return new SchemaDouble(schema);
    if (schema.type === "boolean")
      return new SchemaBool(schema, name);
    if (isJsonObject(schema))
      return new SchemaObject(schema, name, this);
    if (schema.type === "array")
      return new SchemaArray(schema, name, this);

    console.warn(`can't identify type of '${name}', skipping`);
    return undefined;
  }

  create(schema, name, parent) {
    if (parent instanceof SchemaObject) {
      this.#scope.push(parent);
    }

    const override = this.#overrides[name] || {};
    const result = this.#createInScope(schema, override.name || nameUtils.makeTypeName(name));

    if (parent instanceof SchemaObject) {
      this.#scope.pop();
    }

    return result;
  }

  get scope() {
    return [...this.#scope];
  }

}

class EnumValue {
  #baseName;
  #value;
  #intValue;

  constructor(jsonType, value, description) {
    this.#baseName = (jsonType === "string") ? value : description;
    this.#value = (jsonType === "string") ? `"${value}"s` : value;
    this.#intValue = (jsonType === "integer") ? value : undefined;
  }

  get name() {
    return nameUtils.makeIdentifier(this.#baseName);
  }

  get value() {
    return this.#value;
  }

  get modelMember() {
    return (this.#intValue !== undefined) ? `${this.name} = ${this.#intValue}` : this.name;
  }
}

class SchemaEnum {
  #valueType;
  #typeName;
  #valueList;
  #scope;

  #baseObject;

  static fromEnum(schema, name, context) {
    const enm = new SchemaEnum();

    enm.#baseObject = new SchemaObject(schema, name, context);
    enm.#valueType = new SchemaString(schema, name + "Item");
    enm.#typeName = name;
    enm.#valueList = schema.enum.map((item) =>
        new EnumValue(schema.type, item, schema.description))
    enm.#scope = context.scope;

    return enm;
  }

  static fromAnyOf(valueSchema, valueList, name, context) {
    const enm = new SchemaEnum();

    enm.#baseObject = context.create(valueSchema, name, enm);
    enm.#valueType = enm.#baseObject;
    enm.#typeName = name;
    enm.#valueList = valueList
      .filter((item) => item.enum !== undefined)
      .map((item) => new EnumValue(valueSchema.type, item.enum[0], item.description));
    enm.#scope = context.scope;

    return enm;
  }

  get baseName() {
    return this.#typeName;
  }

  get modelHeaders() {
    return this.#valueType.modelHeaders;
  }

  get readerHeaders() {
    return this.#valueType.readerHeaders;
  }

  get model() {
    return nameUtils.makeTypeName(this.#typeName);
  }

  get reader() {
    return this.model + "JsonHandler";
  }

  get scopedModel() {
    return this.#scope
      .concat(this)
      .map((t) => t.model)
      .join("::");
  }

  get scopedReader() {
    return this.#scope
      .concat(this)
      .map((t) => t.reader)
      .join("::");
  }

  get valueType() {
    return this.#valueType;
  }

  get values() {
    return this.#valueList;
  }
}

class Property {
  #name;
  #type;
  #required;

  constructor(name, type, required) {
    this.#name = name;
    this.#type = type;
    this.#required = required;
  }

  get #useOptional() {
    return (this.#type.default === undefined) && !this.#required;
  }

  get name() {
    return this.#name;
  }

  get required() {
    return !this.#useOptional;
  }

  get isAdditional() {
    return this.#type instanceof SchemaAdditionalProperties;
  }

  get modelHeaders() {
    return this.#useOptional ? [...this.#type.modelHeaders, "<optional>"] : this.#type.modelHeaders;
  }

  get readerHeaders() {
    return this.#type.readerHeaders;
  }

  get modelMember() {
    const declType = this.#useOptional
      ? `std::optional<${this.#type.model}>`
      : this.#type.model;
    return (this.#type.default === undefined)
      ? `${declType} ${this.name};`
      : `${declType} ${this.name} = ${this.#type.default};`;
  }

  get readerMember() {
    return `${this.#type.reader} _${this.name};`;
  }

  get modelArgs() {
    return this.#type.modelBase ? this.#type.modelBase.args : "";
  }

  get readerArgs() {
    return this.#type.readerBase ? this.#type.readerBase.args : "";
  }
}

class SchemaAdditionalProperties {
  #propType;

  constructor(schema, context, parent) {
    this.#propType = context.create(schema, "additionalProperty", parent);
  }

  get baseName() {
    return "PropertyMap";
  }

  get propType() {
    return this.#propType;
  }

  get model() {
    return `std::unordered_map<std::string, ${this.#propType.model}>`;
  }

  get reader() {
    return `CesiumJsonReader::DictionaryJsonHandler<${this.#propType.model}, ${this.#propType.reader}>`;
  }

  get modelHeaders() {
    return ["<unordered_map>"];
  }

  get readerHeaders() {
    return ["<CesiumJsonReader/DictionaryJsonHandler.h>"];
  }

  get modelBase() {
    return this.propType.modelBase;
  }

  get readerBase() {
    return this.propType.readerBase;
  }

}

class SchemaRef {
  #ref;
  #context;
  #cachedType;

  constructor(ref, context) {
    this.#ref = ref;
    this.#context = context;
  }

  get #type() {
    if (!this.#cachedType) {
      this.#cachedType = this.#context.resolve(this.#ref);
    }
    return this.#cachedType;
  }

  get default() {
    return this.#type.default;
  }

  get baseName() {
    return this.#type.baseName;
  }

  get model() {
    return this.#type.model;
  }

  get reader() {
    return this.#type.reader;
  }

  get scopedModel() {
    return this.#type.scopedModel;
  }

  get scopedReader() {
    return this.#type.scopedReader;
  }

  get modelHeaders() {
    return [];
  }

  get readerHeaders() {
    return [];
  }

  get modelBase() {
    return this.#type.modelBase;
  }

  get readerBase() {
    return this.#type.readerBase;
  }

  get properties() {
    return this.#type.properties;
  }

  resolve() {
    return this.#type;
  }
}

class TypeInfo {
  #baseName;
  #type;
  #headers;
  #ctor;

  constructor(baseName, type, headers, ctor) {
    this.#baseName = baseName;
    this.#type = type;
    this.#headers = headers || [];
    this.#ctor = ctor || {};
  }

  get baseName() {
    return this.#baseName;
  }

  get type() {
    return this.#type;
  }

  get headers() {
    return this.#headers;
  }

  get params() {
    return Object.keys(this.#ctor)
      .map((arg) => `${this.#ctor[arg]} ${arg}`)
      .join(", ");
  }

  get args() {
    return Object.keys(this.#ctor).join(", ");
  }

  inherit(baseName, type, headers) {
    return new TypeInfo(baseName, type, headers, this.#ctor);
  }
}

class SchemaObject {
  #typeName;
  #schema;
  #context;
  #scope;
  #properties;
  #additionalType;
  #base;

  #modelBase;
  #readerBase;
  #inheritedProps;

  constructor(schema, name, context) {
    console.log("create object", name);
    this.#typeName = name;
    this.#schema = schema;
    this.#context = context;
    this.#scope = context.scope;

    this.#modelBase = new TypeInfo(
      "ExtensibleObject",
      "CesiumUtility::ExtensibleObject",
      ["<CesiumUtility/ExtensibleObject.h>"]
    );
    this.#readerBase = new TypeInfo(
      "ExtensibleObjectJsonHandler",
      "CesiumJsonReader::ExtensibleObjectJsonHandler",
      ["<CesiumJsonReader/ExtensibleObjectJsonHandler.h>"],
      { context: "const ExtensionContext&" }
    );
    const inheritedProps = ["extensions", "extras"];

    const inherits = schema.allOf || [];
    if (inherits.length > 1) {
      console.warn("multiple inheritance not supported");
    }
    if (inherits.length > 0) {
      const base = context.create(inherits[0], name + "Parent");
      this.#modelBase = base.modelBase.inherit(
        base.model,
        base.scopedModel,
        base.modelHeaders
      );
      this.#readerBase = base.readerBase.inherit(
        base.reader,
        base.scopedReader,
        base.readerHeaders
      );
      const baseProps = (base.properties || []).map((p) => p.name);
      inheritedProps.push(...baseProps);

      if (base instanceof SchemaObject) {
        inheritedProps.push(...base.#inheritedProps);
      }

      this.#base = base;
    }
    this.#inheritedProps = lodash.uniq(inheritedProps);

    const propEntries =
      Object.keys(schema.properties || {})
        .filter((name) => !inheritedProps.includes(name))
        .map((name) => [name, context.create(schema.properties[name], name, this)]);
    this.#properties = Object.fromEntries(propEntries);

    if (this.#schema.additionalProperties) {
      this.#additionalType = new SchemaAdditionalProperties(
        this.#schema.additionalProperties, this.#context, this);
    }
  }

  get baseName() {
    return this.#typeName;
  }

  get modelBase() {
    return this.#modelBase;
  }

  get readerBase() {
    return this.#readerBase;
  }

  get model() {
    return this.#typeName;
  }

  get reader() {
    return this.#typeName + "JsonHandler";
  }

  get scopedModel() {
    return this.#scope
      .concat(this)
      .map((t) => t.model)
      .join("::");
  }

  get scopedReader() {
    return this.#scope
      .concat(this)
      .map((t) => t.reader)
      .join("::");
  }

  get modelHeaders() {
    return lodash.uniq(lodash.flatten(
      this.properties.map((t) => t.modelHeaders).concat(this.modelBase.headers)
    ));
  }

  get readerHeaders() {
    return lodash.uniq(lodash.flatten(
      this.properties.map((t) => t.readerHeaders).concat(this.readerBase.headers)
    ));
  }

  #findSubtypes(target) {
    const types = [];
    let queue = Object.values(this.#properties).concat(this.#additionalType);
    while (queue.length > 0) {
      const objs = queue.filter((t) => t instanceof target);
      types.push(...objs);
      queue = queue.map((t) => {
        if (t instanceof SchemaArray)
          return t.elemType;
        if (t instanceof SchemaAdditionalProperties)
          return t.propType;
        if (t instanceof SchemaEnum)
          return t.valueType;
        return undefined;
      }).filter((t) => t);
    }
    if (this.#base instanceof target)
      types.push(this.#base);
    return types;
  }

  get subtypes() {
    return this.#findSubtypes(SchemaObject);
  }

  get enums() {
    return this.#findSubtypes(SchemaEnum);
  }

  get refs() {
    const resolved = {};
    const depGraph = {};

    const queue = [this];
    while (queue.length > 0) {
      const head = queue.pop();
      if (resolved[head.baseName])
        continue;

      const refs = [head, ...head.subtypes].map((t) => t.#findSubtypes(SchemaRef));
      const resolvedRefs = lodash.uniq(lodash.flatten(refs))
        .map((ref) => ref.resolve())
        .filter((ref) => ref.subtypes !== undefined);

      queue.push(...resolvedRefs);
      depGraph[head.baseName] = resolvedRefs.map((t) => t.baseName).filter((name) => name !== head.baseName);
      resolved[head.baseName] = head;
    }

    return topoSort(depGraph).map((name) => resolved[name]);
  }

  get properties() {
    const requiredProps = this.#schema.required || [];
    const props = Object.keys(this.#properties || {})
      .map((name) => {
        const propType = this.#properties[name];
        const required = requiredProps.includes(name);
        return new Property(name, propType, required);
      });

    if (this.#additionalType) {
      const additional = new Property("additionalProperties", this.#additionalType, true);
      props.push(additional);
    }

    return props;
  }

  get hasAdditional() {
    return this.#additionalType !== undefined;
  }
}

class SchemaArray {
  #schema;
  #context;
  #elemType;

  constructor(schema, name, context) {
    this.#schema = schema;
    this.#context = context;
    this.#elemType = context.create(this.#schema.items, name + "Item", this);
  }

  get baseName() {
    return this.#elemType.baseName + "Array";
  }

  get elemType() {
    return this.#elemType;
  }

  get default() {
    return this.#schema.default ? `{ ${this.#schema.default} }` : undefined;
  }

  get model() {
    return `std::vector<${this.elemType.model}>`;
  }

  get reader() {
    return `CesiumJsonReader::ArrayJsonHandler<${this.#elemType.model}, ${this.#elemType.reader}>`;
  }

  get modelHeaders() {
    return lodash.uniq([...this.#elemType.modelHeaders, "<vector>"]);
  }

  get readerHeaders() {
    return lodash.uniq([...this.#elemType.readerHeaders, "<CesiumJsonReader/ArrayJsonHandler.h>"]);
  }

  get modelBase() {
    return this.elemType.modelBase;
  }

  get readerBase() {
    return this.elemType.readerBase;
  }
}

class SchemaInteger {
  #schema;

  constructor(schema) {
    this.#schema = schema;
  }

  get default() {
    return this.#schema.default;
  }

  get baseName() {
    return "Integer";
  }

  get model() {
    return "int64_t";
  }

  get reader() {
    return "CesiumJsonReader::IntegerJsonHandler<int64_t>";
  }

  get modelHeaders() {
    return ["<cstdint>"];
  }

  get readerHeaders() {
    return ["<CesiumJsonReader/IntegerJsonHandler.h>"];
  }
}

class SchemaDouble {
  #schema;

  constructor(schema) {
    this.#schema = schema;
  }

  get default() {
    return this.#schema.default;
  }

  get baseName() {
    return "Double";
  }

  get model() {
    return "double";
  }

  get reader() {
    return "CesiumJsonReader::DoubleJsonHandler";
  }

  get modelHeaders() {
    return [];
  }

  get readerHeaders() {
    return ["<CesiumJsonReader/DoubleJsonHandler.h>"];
  }
}

class SchemaString {
  #schema;

  constructor(schema) {
    this.#schema = schema;
  }

  get default() {
    return this.#schema.default ? `"${this.#schema.default}"` : undefined;
  }

  get baseName() {
    return "String";
  }

  get model() {
    return "std::string";
  }

  get reader() {
    return "CesiumJsonReader::StringJsonHandler";
  }

  get modelHeaders() {
    return ["<string>"];
  }

  get readerHeaders() {
    return ["<CesiumJsonReader/StringJsonHandler.h>"];
  }
}

class SchemaBool {
  #schema;

  constructor(schema) {
    this.#schema = schema;
  }

  get default() {
    return this.#schema.default;
  }

  get baseName() {
    return "Bool";
  }

  get model() {
    return "bool";
  }

  get reader() {
    return "CesiumJsonReader::BoolJsonHandler";
  }

  get modelHeaders() {
    return [];
  }

  get readerHeaders() {
    return ["<CesiumJsonReader/BoolJsonHandler.h>"];
  }
}


function isJsonObject(details) {
  return details.type === "object"
      || details.$schema !== undefined
      || details.properties !== undefined;
}

module.exports = {
  Context: Context,
};
