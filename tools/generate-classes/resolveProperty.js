const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");
const makeIdentifier = require("./makeIdentifier");
const lodash = require("lodash");
const cppReservedWords = require("./cppReservedWords");

function resolveProperty(
  schemaCache,
  config,
  parentName,
  propertyName,
  propertyDetails,
  required,
  namespace
) {
  if (Object.keys(propertyDetails).length === 0) {
    // Ignore totally empty properties. The glTF and 3D Tiles JSON schema files often use empty properties in derived classes
    // when the actual property definition is in the base class.
    return undefined;
  }

  cppSafeName = makeNameIntoValidIdentifier(propertyName);

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
      cppSafeName,
      propertyDetails,
      required,
      namespace
    );
  } else if (propertyDetails.type == "integer") {
    return {
      ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
      headers: ["<cstdint>", ...(makeOptional ? ["<optional>"] : [])],
      type: makeOptional ? "std::optional<int64_t>" : "int64_t",
      readerHeaders: [`<CesiumJsonReader/IntegerJsonHandler.h>`],
      readerType: "CesiumJsonReader::IntegerJsonHandler<int64_t>",
      needsInitialization: !makeOptional,
    };
  } else if (propertyDetails.type == "number") {
    return {
      ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
      headers: makeOptional ? ["<optional>"] : [],
      type: makeOptional ? "std::optional<double>" : "double",
      readerHeaders: [`<CesiumJsonReader/DoubleJsonHandler.h>`],
      readerType: "CesiumJsonReader::DoubleJsonHandler",
      needsInitialization: !makeOptional,
    };
  } else if (propertyDetails.type == "boolean") {
    return {
      ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
      headers: makeOptional ? ["<optional>"] : [],
      type: makeOptional ? "std::optional<bool>" : "bool",
      readerHeaders: `<CesiumJsonReader/BoolJsonHandler.h>`,
      readerType: "CesiumJsonReader::BoolJsonHandler",
      needsInitialization: ~makeOptional,
    };
  } else if (propertyDetails.type == "string") {
    return {
      ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
      type: makeOptional ? "std::optional<std::string>" : "std::string",
      headers: ["<string>", ...(makeOptional ? ["<optional>"] : [])],
      readerHeaders: [`<CesiumJsonReader/StringJsonHandler.h>`],
      readerType: "CesiumJsonReader::StringJsonHandler",
      defaultValue:
        propertyDetails.default !== undefined
          ? `"${propertyDetails.default.toString()}"`
          : undefined,
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
      cppSafeName,
      propertyDetails,
      required,
      namespace
    );
  } else if (isEnum(propertyDetails)) {
    return resolveEnum(
      schemaCache,
      config,
      parentName,
      propertyName,
      cppSafeName,
      propertyDetails,
      makeOptional,
      namespace
    );
  } else if (propertyDetails.$ref) {
    const itemSchema = schemaCache.load(propertyDetails.$ref);
    if (itemSchema.title === "glTF Id") {
      return {
        ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
        type: "int32_t",
        defaultValue: -1,
        headers: ["<cstdint>"],
        readerHeaders: [`<CesiumJsonReader/IntegerJsonHandler.h>`],
        readerType: "CesiumJsonReader::IntegerJsonHandler<int32_t>",
      };
    } else {
      const type = getNameFromSchema(config, itemSchema);
      const typeName = getNameFromSchema(config, itemSchema);

      return {
        ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
        type: makeOptional ? `std::optional<${typeName}>` : typeName,
        headers: [`"${type}.h"`, ...(makeOptional ? ["<optional>"] : [])],
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
      namespace
    );

    return {
      ...nested,
      briefDoc: propertyDefaults(propertyName, cppSafeName, propertyDetails).briefDoc,
      fullDoc: propertyDefaults(propertyName, cppSafeName, propertyDetails).fullDoc,
    };
  } else {
    console.warn(`Cannot interpret property ${propertyName}; using JsonValue.`);
    return {
      ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
      type: `CesiumUtility::JsonValue`,
      headers: [`<CesiumUtility/JsonValue.h>`],
      readerType: `CesiumJsonReader::JsonObjectJsonHandler`,
      readerHeaders: [`<CesiumJsonReader/JsonObjectJsonHandler.h>`],
    };
  }
}

function toPascalCase(name) {
  if (name.length === 0) {
    return name;
  }

  return name[0].toUpperCase() + name.substr(1);
}

function propertyDefaults(propertyName, cppSafeName, propertyDetails) {
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
    cppSafeName: cppSafeName,
    headers: [],
    readerHeaders: [],
    readerHeadersImpl: [],
    type: "",
    defaultValue:
      propertyDetails.default !== undefined
        ? propertyDetails.default.toString()
        : undefined,
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
  cppSafeName,
  propertyDetails,
  required,
  namespace
) {
  // If there is no items definition, pass an effectively empty object.
  // But if the definition is _actually_ empty, the property will be ignored
  // completely. So just add a dummy property.

  const itemProperty = resolveProperty(
    schemaCache,
    config,
    parentName,
    propertyName + ".items",
    propertyDetails.items || { notEmpty: true },
    undefined,
    namespace
  );

  if (!itemProperty) {
    return undefined;
  }

  return {
    ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
    name: propertyName,
    headers: ["<vector>", ...itemProperty.headers],
    schemas: itemProperty.schemas,
    localTypes: itemProperty.localTypes,
    type: `std::vector<${itemProperty.type}>`,
    defaultValue: propertyDetails.default
      ? `{ ${propertyDetails.default} }`
      : undefined,
    readerHeaders: [
      `<CesiumJsonReader/ArrayJsonHandler.h>`,
      ...itemProperty.readerHeaders,
    ],
    readerType: `CesiumJsonReader::ArrayJsonHandler<${itemProperty.type}, ${itemProperty.readerType}>`,
  };
}

function resolveDictionary(
  schemaCache,
  config,
  parentName,
  propertyName,
  cppSafeName,
  propertyDetails,
  required,
  namespace
) {
  const additional = resolveProperty(
    schemaCache,
    config,
    parentName,
    propertyName + ".additionalProperties",
    propertyDetails.additionalProperties,
    // Treat the nested property type as required so it's not wrapped in std::optional.
    [propertyName + ".additionalProperties"],
    namespace
  );

  if (!additional) {
    return undefined;
  }

  return {
    ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
    name: propertyName,
    headers: ["<unordered_map>", ...additional.headers],
    schemas: additional.schemas,
    localTypes: additional.localTypes,
    type: `std::unordered_map<std::string, ${additional.type}>`,
    readerHeaders: [
      `<CesiumJsonReader/DictionaryJsonHandler.h>`,
      ...additional.readerHeaders,
    ],
    readerType: `CesiumJsonReader::DictionaryJsonHandler<${additional.type}, ${additional.readerType}>`,
  };
}

/**
 * @brief Creates a documentation comment block for the class that
 * contains the values for the given enum property.
 *
 * @param {Object} propertyValues The property
 * @return {String} The comment block
 */
function createEnumPropertyDoc(propertyValues) {
  let propertyDoc = `/**\n * @brief Known values for ${
    propertyValues.briefDoc || propertyValues.name
  }\n`;
  propertyDoc += ` */`;
  return propertyDoc;
}

/**
 * Returns a string representing the common type of an "anyOf" property.
 *
 * If there is no common type (because it is not an "anyOf" property,
 * or the "anyOf" entries don't contain "type" properties, or there
 * are different "type" properties), then undefined is returned.
 *
 * @param {String} propertyName The property name
 * @param {Object} propertyDetails The value of the property in the JSON
 * schema. Good to know that. Used a debugger to figure it out.
 * @returns The string indicating the common type, or undefined.
 */
function findCommonEnumType(propertyName, propertyDetails) {
  if (!isEnum(propertyDetails)) {
    return undefined;
  }
  let firstType = undefined;
  for (let i = 0; i < propertyDetails.anyOf.length; i++) {
    const element = propertyDetails.anyOf[i];
    if (element.type) {
      if (firstType) {
        if (element.type !== firstType) {
          console.warn(
            "Expected equal types for enum values in " +
              propertyName +
              ", but found " +
              firstType +
              " and " +
              element.type
          );
          return undefined;
        }
      } else {
        firstType = element.type;
      }
    }
  }
  return firstType;
}

function resolveEnum(
  schemaCache,
  config,
  parentName,
  propertyName,
  cppSafeName,
  propertyDetails,
  makeOptional,
  namespace
) {
  if (!isEnum(propertyDetails)) {
    return undefined;
  }

  const enumType = findCommonEnumType(propertyName, propertyDetails);
  if (!enumType) {
    return undefined;
  }
  const enumRuntimeType = enumType === "string" ? "std::string" : "int32_t";

  const enumName = toPascalCase(propertyName);

  const readerTypes = createEnumReaderType(
    parentName,
    enumName,
    propertyName,
    propertyDetails
  );

  const propertyDefaultValues = propertyDefaults(propertyName, cppSafeName, propertyDetails);
  const enumBriefDoc =
    propertyDefaultValues.briefDoc +
    "\n * \n * Known values are defined in {@link " +
    enumName +
    "}.\n *";
  const result = {
    ...propertyDefaultValues,
    localTypes: [
      unindent(`
        ${createEnumPropertyDoc(propertyDefaultValues)}
        struct ${enumName} {
            ${indent(
              propertyDetails.anyOf
                .map((e) => createEnum(e))
                .filter((e) => e !== undefined)
                .join(";\n\n") + ";",
              12
            )}
        };
      `),
    ],
    type: makeOptional ? `std::optional<${enumRuntimeType}>` : enumRuntimeType,
    headers: makeOptional ? ["<optional>"] : [],
    defaultValue: makeOptional
      ? undefined
      : createEnumDefault(enumName, propertyDetails),
    readerHeaders: [`<${namespace}/${parentName}.h>`],
    readerLocalTypes: readerTypes,
    readerLocalTypesImpl: createEnumReaderTypeImpl(
      parentName,
      enumName,
      propertyName,
      propertyDetails
    ),
    needsInitialization: !makeOptional,
    briefDoc: enumBriefDoc,
  };

  if (readerTypes.length > 0) {
    result.readerType = `${enumName}JsonHandler`;
  } else if (enumType === "integer") {
    result.readerType = `CesiumJsonReader::IntegerJsonHandler<${enumRuntimeType}>`;
    result.readerHeaders.push(`<CesiumJsonReader/IntegerJsonHandler.h>`);
  } else if (enumType === "string") {
    result.readerType = `CesiumJsonReader::StringJsonHandler`;
    result.readerHeaders.push(`<CesiumJsonReader/StringJsonHandler.h>`);
  }

  return result;
}

function getEnumValue(enumDetails) {
  if (enumDetails.const !== undefined) {
    return enumDetails.const;
  }

  if (enumDetails.enum !== undefined && enumDetails.enum.length > 0) {
    return enumDetails.enum[0];
  }

  return undefined;
}

function isEnum(propertyDetails) {
  if (
    propertyDetails.anyOf === undefined ||
    propertyDetails.anyOf.length === 0
  ) {
    return false;
  }

  const firstEnum = propertyDetails.anyOf[0];
  const firstEnumValue = getEnumValue(firstEnum);

  return firstEnumValue !== undefined;
}

/**
 * If ... many conditions hold, ... then this will return the name
 * of the "initial" value of the given enum property.
 *
 * @param {Object} propertyDetails The property details
 * @returns The name of the default property
 */
function findNameOfInitial(propertyDetails) {
  if (propertyDetails.default !== undefined) {
    for (let i = 0; i < propertyDetails.anyOf.length; i++) {
      const element = propertyDetails.anyOf[i];
      const enumValue = getEnumValue(element);
      if (enumValue === propertyDetails.default) {
        if (element.type === "integer") {
          return element.description;
        }
        return `${makeIdentifier(enumValue)}`;
      }
    }
  }
  // No explicit default value was found. Return the first value
  for (let i = 0; i < propertyDetails.anyOf.length; i++) {
    const element = propertyDetails.anyOf[i];
    const enumValue = getEnumValue(element);
    if (enumValue !== undefined) {
      if (element.type === "integer") {
        return element.description;
      }
      return `${makeIdentifier(enumValue)}`;
    }
  }
  return undefined;
}

function createEnumDefault(enumName, propertyDetails) {
  return `${enumName}::${findNameOfInitial(propertyDetails)}`;
}

function createEnum(enumDetails) {
  const enumValue = getEnumValue(enumDetails);
  if (enumValue === undefined) {
    return undefined;
  }

  if (enumDetails.type === "integer") {
    return `static constexpr int32_t ${makeIdentifier(
      enumDetails.description
    )} = ${enumValue}`;
  } else {
    return `inline static const std::string ${makeIdentifier(
      enumValue
    )} = \"${enumValue}\"`;
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
  if (findCommonEnumType(propertyName, propertyDetails) === "string") {
    // No special reader needed for string enums.
    return [];
  }

  return unindent(`
    class ${enumName}JsonHandler : public CesiumJsonReader::JsonHandler {
    public:
      ${enumName}JsonHandler() noexcept : CesiumJsonReader::JsonHandler() {}
      void reset(CesiumJsonReader::IJsonHandler* pParent, ${parentName}::${enumName}* pEnum);
      virtual CesiumJsonReader::IJsonHandler* readString(const std::string_view& str) override;

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
  if (findCommonEnumType(propertyName, propertyDetails) === "string") {
    // No special reader needed for string enums.
    return [];
  }

  return unindent(`
    void ${parentName}JsonHandler::${enumName}JsonHandler::reset(CesiumJsonReader::IJsonHandler* pParent, ${parentName}::${enumName}* pEnum) {
      JsonHandler::reset(pParent);
      this->_pEnum = pEnum;
    }

    CesiumJsonReader::IJsonHandler* ${parentName}JsonHandler::${enumName}JsonHandler::readString(const std::string_view& str) {
      using namespace std::string_literals;

      assert(this->_pEnum);

      ${indent(
        propertyDetails.anyOf
          .map((e) => {
            const enumValue = getEnumValue(e);
            return enumValue !== undefined
              ? `if ("${enumValue}"s == str) *this->_pEnum = ${parentName}::${enumName}::${makeIdentifier(
                  enumValue
                )};`
              : undefined;
          })
          .filter((s) => s !== undefined)
          .join("\nelse "),
        6
      )}
      else return nullptr;

      return this->parent();
    }
  `);
}

function makeNameIntoValidIdentifier(name) {
  if (cppReservedWords.indexOf(name) >= 0) {
    name += "Property";
  }
  return name;
}

module.exports = resolveProperty;
