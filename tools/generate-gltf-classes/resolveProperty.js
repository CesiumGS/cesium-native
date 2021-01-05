const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");
const makeIdentifier = require("./makeIdentifier");

function resolveProperty(
  schemaCache,
  nameMapping,
  parentName,
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
      parentName,
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
      type: "std::string",
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
      parentName,
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
      parentName,
      propertyName,
      propertyDetails
    );
  } else if (propertyDetails.$ref) {
    const itemSchema = schemaCache.load(propertyDetails.$ref);
    if (itemSchema.title === "glTF Id") {
      return {
        ...propertyDefaults(propertyName, propertyDetails),
        type: "int32_t",
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
      parentName,
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
    readerLocalTypesImpl: [],
    briefDoc: propertyDetails.description,
    fullDoc: propertyDetails.gltf_detailedDescription,
  };
}

function resolveArray(schemaCache, nameMapping, parentName, propertyName, propertyDetails) {
  const itemProperty = resolveProperty(
    schemaCache,
    nameMapping,
    parentName,
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

function resolveDictionary(schemaCache, nameMapping, parentName, propertyName, propertyDetails) {
  const additional = resolveProperty(
    schemaCache,
    nameMapping,
    parentName,
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

function resolveEnum(schemaCache, nameMapping, parentName, propertyName, propertyDetails) {
  if (!propertyDetails.anyOf || propertyDetails.anyOf.length === 0 || !propertyDetails.anyOf[0].enum || propertyDetails.anyOf[0].enum.length === 0) {
    return undefined;
  }

  const enumName = toPascalCase(propertyName);

  const readerTypes = createEnumReaderType(enumName, propertyName, propertyDetails);

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
              12
            )}
        };
      `)
    ],
    type: enumName,
    readerLocalTypes: readerTypes,
    readerLocalTypesImpl: createEnumReaderTypeImpl(parentName, enumName, propertyName, propertyDetails),
    readerType: readerTypes.length > 0 ? `${enumName}JsonHandler` : "IntegerJsonHandler<int32_t>"
  };
}

function createEnum(enumDetails) {
  if (!enumDetails.enum || enumDetails.enum.length === 0) {
    return undefined;
  }

  if (enumDetails.type === "integer") {
    return `${makeIdentifier(enumDetails.description)} = ${enumDetails.enum[0]}`;
  } else {
    return makeIdentifier(enumDetails.enum[0]);
  }
}

function createEnumReaderType(enumName, propertyName, propertyDetails) {
  if (propertyDetails.anyOf[0].type === "integer") {
    // No special reader needed for integer enums.
    return [];
  }

  return unindent(`
    class ${enumName}JsonHandler : public JsonHandler {
    public:
      void reset(JsonHandler* pParent, ${enumName}* pEnum);
      virtual JsonHandler* String(const char* str, size_t length, bool copy) override;

    private:
      ${enumName}* _pEnum = nullptr;
    };
  `);
}

function createEnumReaderTypeImpl(parentName, enumName, propertyName, propertyDetails) {
  if (propertyDetails.anyOf[0].type === "integer") {
    // No special reader needed for integer enums.
    return [];
  }

  return unindent(`
    void ${parentName}JsonHandler::${enumName}JsonHandler::reset(JsonHandler* pParent, ${enumName}* pEnum) {
      JsonHandler::reset(pParent);
      this->_pEnum = pEnum;
    }

    JsonHandler* ${parentName}JsonHandler::${enumName}JsonHandler::String(const char* str, size_t length, bool copy) {
      using namespace std::string_literals;

      assert(this->_pEnum);

      ${indent(
        propertyDetails.anyOf
          .map((e) => e.enum && e.enum[0] !== undefined ? `if ("${e.enum[0]}"s == str) *this->_pEnum = ${enumName}::${makeIdentifier(e.enum[0])};` : undefined)
          .filter(s => s !== undefined)
          .join("\nelse "),
        6
      )}
      else return nullptr;

      return this->parent();
    }
  `);
}

module.exports = resolveProperty;
