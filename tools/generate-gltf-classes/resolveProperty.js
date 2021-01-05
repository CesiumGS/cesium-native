const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");
const { result } = require("lodash");

function resolveProperty(
  schemaCache,
  nameMapping,
  propertyName,
  propertyDetails
) {
  if (Object.keys(propertyDetails).length === 0) {
    // Ignore totally empty properties.
    return undefined;
  }

  if (propertyDetails.type == "array") {
    return resolveArray(
      schemaCache,
      nameMapping,
      propertyName,
      propertyDetails
    );
  } else if (propertyDetails.type == "integer") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      headers: ["<cstdint>"],
      type: "int64_t",
      readerHeaders: [`"IntegerJsonHandler.h"`],
      readerType: "IntegerJsonHandler<int64_t>"
    };
  } else if (propertyDetails.type == "number") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      type: "double",
      readerHeaders: [`"DoubleJsonHandler.h"`],
      readerType: "DoubleJsonHandler",
    };
  } else if (propertyDetails.type == "boolean") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      type: "bool",
      readerHeaders: `"BoolJsonHandler.h"`,
      readerType: "BoolJsonHandler",
    };
  } else if (propertyDetails.type == "string") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      type: "bool",
      headers: ["<string>"],
      readerHeaders: [`"StringJsonHandler.h"`],
      readerType: "StringJsonHandler",
    };
  } else if (
    propertyDetails.type == "object" &&
    propertyDetails.additionalProperties
  ) {
    return resolveDictionary(
      schemaCache,
      nameMapping,
      propertyName,
      propertyDetails
    );
  } else if (
    propertyDetails.anyOf &&
    propertyDetails.anyOf.length > 0 &&
    propertyDetails.anyOf[0].enum
  ) {
    return resolveEnum(
      schemaCache,
      nameMapping,
      propertyName,
      propertyDetails
    );
  } else if (propertyDetails.$ref) {
    const itemSchema = schemaCache.load(propertyDetails.$ref);
    if (itemSchema.title === "glTF Id") {
      return {
        ...propertyDefaults(propertyName, propertyDetails),
        type: "bool",
        headers: ["<cstdint>"],
        readerHeaders: [`"IntegerJsonHandler.h"`],
        readerType: "IntegerJsonHandler<int32_t>",
      };
    } else {
      const type = getNameFromSchema(nameMapping, itemSchema);
      return {
        ...propertyDefaults(propertyName, propertyDetails),
        type: getNameFromSchema(nameMapping, itemSchema),
        headers: [`"CesiumGltf/${type}.h"`],
        readerType: `${type}JsonHandler`,
        readerHeaders: [`"${type}JsonHandler.h"`],
        schemas: [itemSchema]
      };
    }
  } else if (propertyDetails.allOf && propertyDetails.allOf.length == 1) {
    const nested = resolveProperty(
      schemaCache,
      nameMapping,
      propertyName,
      propertyDetails.allOf[0]
    );
    nested.briefDoc = propertyDetails.description;
    nested.fullDoc = propertyDetails.gltf_detailedDescription;
    return nested;
  } else {
    console.warn(`Skipping unhandled property ${propertyName}.`);
    return undefined;
  }
}

function toPascalCase(name) {
  if (name.length === 0) {
    return name;
  }

  return name[0].toUpperCase() + name.substr(1);
}

function createEnum(enumDetails) {
  if (!enumDetails.enum || enumDetails.enum.length === 0) {
    return undefined;
  }

  if (typeof enumDetails.enum[0] === "string") {
    return enumDetails.enum[0];
  } else {
    return `${enumDetails.description} = ${enumDetails.enum[0]}`;
  }
}

function propertyDefaults(propertyName, propertyDetails) {
  return {
    name: propertyName,
    headers: [],
    readerHeaders: [],
    type: "",
    readerType: "",
    schemas: [],
    localTypes: [],
    readerLocalTypes: [],
    briefDoc: propertyDetails.description,
    fullDoc: propertyDetails.gltf_detailedDescription,
  };
}

function resolveArray(schemaCache, nameMapping, propertyName, propertyDetails) {
  const itemProperty = resolveProperty(
    schemaCache,
    nameMapping,
    propertyName + ".items",
    propertyDetails.items
  );

  if (!itemProperty) {
    return undefined;
  }

  return {
    ...propertyDefaults(propertyName, propertyDetails),
    name: propertyName,
    headers: ["<vector>", ...itemProperty.headers],
    schemas: itemProperty.schemas,
    localTypes: itemProperty.localTypes,
    type: `std::vector<${itemProperty.type}>`,
    readerHeaders: [`"ArrayJsonHandler.h"`, ...itemProperty.readerHeaders],
    readerType: `ArrayJsonHandler<${itemProperty.type}>`
  };
}

function resolveDictionary(schemaCache, nameMapping, propertyName, propertyDetails) {
  const additional = resolveProperty(
    schemaCache,
    nameMapping,
    propertyName + ".additionalProperties",
    propertyDetails.additionalProperties
  );

  if (!additional) {
    return undefined;
  }

  return {
    ...propertyDefaults(propertyName, propertyDetails),
    name: propertyName,
    headers: ["<unordered_map>", ...additional.headers],
    schema: additional.schemas,
    localTypes: additional.localTypes,
    type: `std::unordered_map<std::string, ${additional.type}>`,
    readerHeaders: [`"DictionaryJsonHandler.h"`, ...additional.readerHeaders],
    readerType: `DictionaryJsonHandler<${additional.type}, ${additional.readerType}>`
  };
}

function resolveEnum(schemaCache, nameMapping, propertyName, propertyDetails) {
  const enumName = toPascalCase(propertyName);

  return {
    ...propertyDefaults(propertyName, propertyDetails),
    localTypes: [
      unindent(`
        enum class ${toPascalCase(propertyName)} {
            ${indent(
              propertyDetails.anyOf
                .map((e) => createEnum(e))
                .filter((e) => e !== undefined)
                .join(",\n\n"),
              8
            )}
        };
      `)
    ],
    type: enumName,
    readerLocalTypes: [
      "// TODO: enum handler"
    ],
    readerType: `${enumName}JsonHandler`
  };
}
module.exports = resolveProperty;
