const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");
const makeIdentifier = require("./makeIdentifier");

function resolveProperty(
  schemaCache,
  config,
  parentName,
  propertyName,
  propertyDetails,
  required
) {
  if (Object.keys(propertyDetails).length === 0) {
    // Ignore totally empty properties.
    return undefined;
  }

  // If we don't know what's required, act as if everything is.
  // Specifically this means we _don't_ make it optional.
  const isRequired = required === undefined || required.includes(propertyName);
  const makeOptional = !isRequired && propertyDetails.default === undefined;

  if (propertyDetails.type == "array") {
    return resolveArray(
      schemaCache,
      config,
      parentName,
      propertyName,
      propertyDetails,
      required
    );
  } else if (propertyDetails.type == "integer") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      headers: ["<cstdint>", ...(makeOptional ? ["<optional>"] : [])],
      type: makeOptional ? "std::optional<int64_t>" : "int64_t",
      readerHeaders: [`"IntegerJsonHandler.h"`],
      readerType: "IntegerJsonHandler<int64_t>",
      needsInitialization: !makeOptional
    };
  } else if (propertyDetails.type == "number") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      headers: makeOptional ? ["<optional>"] : [],
      type: makeOptional ? "std::optional<double>" : "double",
      readerHeaders: [`"DoubleJsonHandler.h"`],
      readerType: "DoubleJsonHandler",
      needsInitialization: !makeOptional
    };
  } else if (propertyDetails.type == "boolean") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      headers: makeOptional ? ["<optional>"] : [],
      type: makeOptional ? "std::optional<bool>" : "bool",
      readerHeaders: `"BoolJsonHandler.h"`,
      readerType: "BoolJsonHandler",
      needsInitialization: ~makeOptional
    };
  } else if (propertyDetails.type == "string") {
    return {
      ...propertyDefaults(propertyName, propertyDetails),
      type: makeOptional ? "std::optional<std::string>" : "std::string",
      headers: ["<string>", ...(makeOptional ? ["<optional>"] : [])],
      readerHeaders: [`"StringJsonHandler.h"`],
      readerType: "StringJsonHandler",
    };
  } else if (
    propertyDetails.type == "object" &&
    propertyDetails.additionalProperties
  ) {
    return resolveDictionary(
      schemaCache,
      config,
      parentName,
      propertyName,
      propertyDetails,
      required
    );
  } else if (
    propertyDetails.anyOf &&
    propertyDetails.anyOf.length > 0 &&
    propertyDetails.anyOf[0].enum
  ) {
    return resolveEnum(
      schemaCache,
      config,
      parentName,
      propertyName,
      propertyDetails,
      makeOptional
    );
  } else if (propertyDetails.$ref) {
    const itemSchema = schemaCache.load(propertyDetails.$ref);
    if (itemSchema.title === "glTF Id") {
      return {
        ...propertyDefaults(propertyName, propertyDetails),
        type: "int32_t",
        defaultValue: -1,
        headers: ["<cstdint>"],
        readerHeaders: [`"IntegerJsonHandler.h"`],
        readerType: "IntegerJsonHandler<int32_t>",
      };
    } else {
      const type = getNameFromSchema(config, itemSchema);
      const typeName = getNameFromSchema(config, itemSchema);

      return {
        ...propertyDefaults(propertyName, propertyDetails),
        type: makeOptional ? `std::optional<${typeName}>` : typeName,
        headers: [`"CesiumGltf/${type}.h"`, ...(makeOptional ? ["<optional>"] : [])],
        readerType: `${type}JsonHandler`,
        readerHeaders: [`"${type}JsonHandler.h"`],
        schemas: [itemSchema],
      };
    }
  } else if (propertyDetails.allOf && propertyDetails.allOf.length == 1) {
    const nested = resolveProperty(
      schemaCache,
      config,
      parentName,
      propertyName,
      propertyDetails.allOf[0],
      required,
    );

    return {
      ...nested,
      briefDoc: propertyDefaults(propertyName, propertyDetails).briefDoc,
      fullDoc: propertyDefaults(propertyName, propertyDetails).fullDoc,
    };
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
  const fullDoc =
    propertyDetails.gltf_detailedDescription &&
    propertyDetails.gltf_detailedDescription.indexOf(
      propertyDetails.description
    ) === 0
      ? propertyDetails.gltf_detailedDescription
          .substr(propertyDetails.description.length)
          .trim()
      : propertyDetails.gltf_detailedDescription;
  return {
    name: propertyName,
    headers: [],
    readerHeaders: [],
    readerHeadersImpl: [],
    type: "",
    defaultValue: propertyDetails.default !== undefined ? propertyDetails.default.toString() : undefined,
    readerType: "",
    schemas: [],
    localTypes: [],
    readerLocalTypes: [],
    readerLocalTypesImpl: [],
    briefDoc: propertyDetails.description,
    fullDoc: fullDoc,
  };
}

function resolveArray(
  schemaCache,
  config,
  parentName,
  propertyName,
  propertyDetails,
  required
) {
  const itemProperty = resolveProperty(
    schemaCache,
    config,
    parentName,
    propertyName + ".items",
    propertyDetails.items,
    undefined
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
    defaultValue: propertyDetails.default ? `{ ${propertyDetails.default} }` : undefined,
    readerHeaders: [`"ArrayJsonHandler.h"`, ...itemProperty.readerHeaders],
    readerType: `ArrayJsonHandler<${itemProperty.type}, ${itemProperty.readerType}>`,
  };
}

function resolveDictionary(
  schemaCache,
  config,
  parentName,
  propertyName,
  propertyDetails,
  required
) {
  const additional = resolveProperty(
    schemaCache,
    config,
    parentName,
    propertyName + ".additionalProperties",
    propertyDetails.additionalProperties,
    required
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
    readerType: `DictionaryJsonHandler<${additional.type}, ${additional.readerType}>`,
  };
}

/**
 * @brief Creates a documentation comment block for the given property.
 * 
 * The result will be a (non-indented) doxygen comment block that contains 
 * the `briefDoc` (or `name`) and `fullDoc` from the given property values.
 * 
 * @param {Object} propertyValues The property
 * @return {String} The comment block
 */
function createPropertyDoc(propertyValues) {
  let propertyDoc = `/**\n * @brief ${propertyValues.briefDoc || propertyValues.name}\n`;
  if (propertyValues.fullDoc) {
    propertyDoc += ` *\n * ${propertyValues.fullDoc.split("\n").join("\n * ")}\n`;
  }
  propertyDoc += ` */`;
  return propertyDoc;
}
 

function resolveEnum(
  schemaCache,
  config,
  parentName,
  propertyName,
  propertyDetails,
  makeOptional
) {
  if (
    !propertyDetails.anyOf ||
    propertyDetails.anyOf.length === 0 ||
    !propertyDetails.anyOf[0].enum ||
    propertyDetails.anyOf[0].enum.length === 0
  ) {
    return undefined;
  }

  const enumName = toPascalCase(propertyName);

  const readerTypes = createEnumReaderType(
    parentName,
    enumName,
    propertyName,
    propertyDetails
  );

  const propertyDefaultValues = propertyDefaults(propertyName, propertyDetails);
  const result = {
    ...propertyDefaultValues,
    localTypes: [
      unindent(`
        ${createPropertyDoc(propertyDefaultValues)}
        enum class ${toPascalCase(propertyName)} {
            ${indent(
              propertyDetails.anyOf
                .map((e) => createEnum(e))
                .filter((e) => e !== undefined)
                .join(",\n\n"),
              12
            )}
        };
      `),
    ],
    type: makeOptional ? `std::optional<${enumName}>` : enumName,
    headers: makeOptional ? ["<optional>"] : [],
    defaultValue: createEnumDefault(enumName, propertyDetails),
    readerHeaders: [`"CesiumGltf/${parentName}.h"`],
    readerLocalTypes: readerTypes,
    readerLocalTypesImpl: createEnumReaderTypeImpl(
      parentName,
      enumName,
      propertyName,
      propertyDetails
    ),
    needsInitialization: !makeOptional
  };

  if (readerTypes.length > 0) {
    result.readerType = `${enumName}JsonHandler`;
  } else {
    result.readerType = `IntegerJsonHandler<${parentName}::${enumName}>`;
    result.readerHeaders.push(`"IntegerJsonHandler.h"`);
  }

  return result;
}

function createEnumDefault(enumName, propertyDetails) {
  if (propertyDetails.default === undefined) {
    return undefined;
  }

  if (propertyDetails.anyOf[0].type === "integer") {
    return `${enumName}(${propertyDetails.default})`;
  } else {
    return `${enumName}::${propertyDetails.default}`;
  }
}

function createEnum(enumDetails) {
  if (!enumDetails.enum || enumDetails.enum.length === 0) {
    return undefined;
  }

  if (enumDetails.type === "integer") {
    return `${makeIdentifier(enumDetails.description)} = ${
      enumDetails.enum[0]
    }`;
  } else {
    return makeIdentifier(enumDetails.enum[0]);
  }
}

function createEnumReaderType(
  parentName,
  enumName,
  propertyName,
  propertyDetails
) {
  if (propertyDetails.anyOf[0].type === "integer") {
    // No special reader needed for integer enums.
    return [];
  }

  return unindent(`
    class ${enumName}JsonHandler : public JsonHandler {
    public:
      ${enumName}JsonHandler(const ReadModelOptions& options) noexcept : JsonHandler(options) {}
      void reset(IJsonHandler* pParent, ${parentName}::${enumName}* pEnum);
      virtual IJsonHandler* readString(const std::string_view& str) override;

    private:
      ${parentName}::${enumName}* _pEnum = nullptr;
    };
  `);
}

function createEnumReaderTypeImpl(
  parentName,
  enumName,
  propertyName,
  propertyDetails
) {
  if (propertyDetails.anyOf[0].type === "integer") {
    // No special reader needed for integer enums.
    return [];
  }

  return unindent(`
    void ${parentName}JsonHandler::${enumName}JsonHandler::reset(IJsonHandler* pParent, ${parentName}::${enumName}* pEnum) {
      JsonHandler::reset(pParent);
      this->_pEnum = pEnum;
    }

    IJsonHandler* ${parentName}JsonHandler::${enumName}JsonHandler::readString(const std::string_view& str) {
      using namespace std::string_literals;

      assert(this->_pEnum);

      ${indent(
        propertyDetails.anyOf
          .map((e) =>
            e.enum && e.enum[0] !== undefined
              ? `if ("${
                  e.enum[0]
                }"s == str) *this->_pEnum = ${parentName}::${enumName}::${makeIdentifier(
                  e.enum[0]
                )};`
              : undefined
          )
          .filter((s) => s !== undefined)
          .join("\nelse "),
        6
      )}
      else return nullptr;

      return this->parent();
    }
  `);
}

module.exports = resolveProperty;
