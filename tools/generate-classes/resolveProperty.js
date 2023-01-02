const cppReservedWords = require("./cppReservedWords");
const getNameFromTitle = require("./getNameFromTitle");
const indent = require("./indent");
const makeIdentifier = require("./makeIdentifier");
const NameFormatters = require("./NameFormatters");
const unindent = require("./unindent");

function resolveProperty(
  schemaCache,
  config,
  parentSchema,
  parentName,
  propertyName,
  propertyDetails,
  required,
  namespace,
  readerNamespace,
  writerNamespace
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

  if (isEnum(propertyDetails)) {
    return resolveEnum(
      schemaCache,
      config,
      parentName,
      propertyName,
      cppSafeName,
      propertyDetails,
      isRequired,
      makeOptional,
      namespace,
      readerNamespace,
      writerNamespace
    );
  } else if (propertyDetails.type == "array") {
    return resolveArray(
      schemaCache,
      config,
      parentSchema,
      parentName,
      propertyName,
      cppSafeName,
      propertyDetails,
      required,
      namespace,
      readerNamespace,
      writerNamespace
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
  } else if (propertyDetails.type === "object" && propertyDetails.properties) {
    // This is an anonymous, inline object definition
    const name = createAnonymousPropertyTypeTitle(parentName, propertyName);
    const schema = {
      ...propertyDetails,
      title: name,
      sourcePath: parentSchema.sourcePath,
    };
    const type = getNameFromTitle(config, schema.title);
    return {
      ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
      type: makeOptional
        ? `std::optional<${NameFormatters.getName(type, namespace)}>`
        : `${NameFormatters.getName(type, namespace)}`,
      headers: [
        NameFormatters.getIncludeFromName(type, namespace),
        ...(makeOptional ? ["<optional>"] : []),
      ],
      readerType: NameFormatters.getReaderName(type, readerNamespace),
      readerHeaders: [
        NameFormatters.getReaderIncludeFromName(type, readerNamespace),
      ],
      schemas: [schema],
    };
  } else if (
    propertyDetails.type === "object" &&
    propertyDetails.additionalProperties
  ) {
    return resolveDictionary(
      schemaCache,
      config,
      parentSchema,
      parentName,
      propertyName,
      cppSafeName,
      propertyDetails,
      required,
      namespace,
      readerNamespace,
      writerNamespace
    );
  } else if (propertyDetails.$ref) {
    let itemSchema = schemaCache.load(propertyDetails.$ref);

    // Get a "sub-schema" when the $ref points to an inner key.
    // e.g. definitions.schema.json#/definitions/numericValue
    const innerKeyMarker = "#/";
    const innerKeyIndex = propertyDetails.$ref.indexOf(innerKeyMarker);
    if (
      innerKeyIndex != -1 &&
      innerKeyIndex + innerKeyMarker.length < propertyDetails.$ref.length
    ) {
      const innerKeys = propertyDetails.$ref
        .slice(innerKeyIndex + innerKeyMarker.length)
        .split("/");
      for (let key of innerKeys) {
        itemSchema = itemSchema[key];
      }
    }
    if (itemSchema.title === "glTF Id") {
      return {
        ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
        type: "int32_t",
        defaultValue: -1,
        headers: ["<cstdint>"],
        readerHeaders: [`<CesiumJsonReader/IntegerJsonHandler.h>`],
        readerType: "CesiumJsonReader::IntegerJsonHandler<int32_t>",
        requiredId: isRequired,
      };
    } else if (itemSchema.type !== undefined && itemSchema.type !== "object") {
      return resolveProperty(
        schemaCache,
        config,
        parentSchema,
        parentName,
        propertyName,
        itemSchema,
        required,
        namespace,
        readerNamespace,
        writerNamespace
      );
    } else {
      const type = getNameFromTitle(config, itemSchema.title);
      if (type === "CesiumUtility::JsonValue") {
        return makeJsonValueProperty(
          propertyName,
          cppSafeName,
          propertyDetails,
          makeOptional
        );
      } else {
        return {
          ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
          type: makeOptional
            ? `std::optional<${NameFormatters.getName(type, namespace)}>`
            : `${NameFormatters.getName(type, namespace)}`,
          headers: [
            NameFormatters.getIncludeFromName(type, namespace),
            ...(makeOptional ? ["<optional>"] : []),
          ],
          readerType: NameFormatters.getReaderName(type, readerNamespace),
          readerHeaders: [
            NameFormatters.getReaderIncludeFromName(type, readerNamespace),
          ],
          schemas: [itemSchema],
        };
      }
    }
  } else if (propertyDetails.allOf && propertyDetails.allOf.length == 1) {
    const nested = resolveProperty(
      schemaCache,
      config,
      parentSchema,
      parentName,
      propertyName,
      propertyDetails.allOf[0],
      required,
      namespace,
      readerNamespace,
      writerNamespace
    );

    return {
      ...nested,
      briefDoc: propertyDefaults(propertyName, cppSafeName, propertyDetails)
        .briefDoc,
      fullDoc: propertyDefaults(propertyName, cppSafeName, propertyDetails)
        .fullDoc,
    };
  } else {
    return makeJsonValueProperty(
      propertyName,
      cppSafeName,
      propertyDetails,
      makeOptional
    );
  }
}

function makeJsonValueProperty(
  propertyName,
  cppSafeName,
  propertyDetails,
  makeOptional
) {
  console.warn(`Cannot interpret property ${propertyName}; using JsonValue.`);
  return {
    ...propertyDefaults(propertyName, cppSafeName, propertyDetails),
    type: makeOptional
      ? `std::optional<CesiumUtility::JsonValue>`
      : `CesiumUtility::JsonValue`,
    headers: [
      `<CesiumUtility/JsonValue.h>`,
      ...(makeOptional ? ["<optional>"] : []),
    ],
    readerType: `CesiumJsonReader::JsonObjectJsonHandler`,
    readerHeaders: [`<CesiumJsonReader/JsonObjectJsonHandler.h>`],
  };
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
  parentSchema,
  parentName,
  propertyName,
  cppSafeName,
  propertyDetails,
  required,
  namespace,
  readerNamespace,
  writerNamespace
) {
  // If there is no items definition, pass an effectively empty object.
  // But if the definition is _actually_ empty, the property will be ignored
  // completely. So just add a dummy property.

  const itemProperty = resolveProperty(
    schemaCache,
    config,
    parentSchema,
    parentName,
    propertyName + ".items",
    propertyDetails.items || { notEmpty: true },
    undefined,
    namespace,
    readerNamespace,
    writerNamespace
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
  parentSchema,
  parentName,
  propertyName,
  cppSafeName,
  propertyDetails,
  required,
  namespace,
  readerNamespace,
  writerNamespace
) {
  const additional = resolveProperty(
    schemaCache,
    config,
    parentSchema,
    parentName,
    propertyName + ".additionalProperties",
    propertyDetails.additionalProperties,
    // Treat the nested property type as required so it's not wrapped in std::optional.
    [propertyName + ".additionalProperties"],
    namespace,
    readerNamespace,
    writerNamespace
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
 * Returns a string representing the common type of an enum.
 *
 * If there is no common type (because "type" is undefined, or there
 * are different "type" properties), then undefined is returned.
 *
 * @param {String} propertyName The property name
 * @param {Array} enums The enums.
 * @returns The string indicating the common type, or undefined.
 */
function findCommonEnumType(propertyName, enums) {
  let firstType = undefined;
  for (let i = 0; i < enums.length; i++) {
    const element = enums[i];
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
  isRequired,
  makeOptional,
  namespace,
  readerNamespace,
  writerNamespace
) {
  if (!isEnum(propertyDetails)) {
    return undefined;
  }

  // Convert the three enum variations into a common format
  const enums = [];
  if (propertyDetails.enum) {
    for (const e of propertyDetails.enum) {
      enums.push({
        cppSafeName: makeNameIntoValidEnumIdentifier(e),
        const: e,
        description: e,
        type: propertyDetails.type,
      });
    }
  } else if (propertyDetails.anyOf) {
    for (const e of propertyDetails.anyOf) {
      if (e.enum !== undefined && e.enum.length > 0) {
        enums.push({
          cppSafeName: makeNameIntoValidEnumIdentifier(e.enum[0]),
          const: e.enum[0],
          description: e.description,
          type: propertyDetails.type,
        });
      } else {
        enums.push({
          cppSafeName: makeNameIntoValidEnumIdentifier(e.const),
          const: e.const,
          description: e.description,
          type: e.type,
        });
      }
    }
  }

  const enumType = findCommonEnumType(propertyName, enums);
  if (!enumType) {
    return undefined;
  }
  const enumRuntimeType = enumType === "string" ? "std::string" : "int32_t";

  const enumName = toPascalCase(propertyName);
  const enumDefaultValue = createEnumDefault(enumName, propertyDetails, enums);
  const enumDefaultValueWriter = `${namespace}::${parentName}::${enumDefaultValue}`;

  const readerTypes = createEnumReaderType(
    parentName,
    enumName,
    propertyName,
    enums
  );

  const propertyDefaultValues = propertyDefaults(
    propertyName,
    cppSafeName,
    propertyDetails
  );
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
              enums
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
    defaultValue: makeOptional ? undefined : enumDefaultValue,
    defaultValueWriter: makeOptional ? undefined : enumDefaultValueWriter,
    readerHeaders: [`<${namespace}/${parentName}.h>`],
    readerLocalTypes: readerTypes,
    readerLocalTypesImpl: createEnumReaderTypeImpl(
      parentName,
      enumName,
      propertyName,
      enums
    ),
    needsInitialization: !makeOptional,
    briefDoc: enumBriefDoc,
    requiredEnum: isRequired,
  };

  if (enumType === "string") {
    result.headers.push("<string>");
  }

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

  return undefined;
}

function isEnum(propertyDetails) {
  if (propertyDetails.anyOf && propertyDetails.anyOf.length > 0) {
    const firstEnum = propertyDetails.anyOf[0];
    const firstEnumValue = getEnumValue(firstEnum);

    return firstEnumValue !== undefined;
  }

  // Enum form seen in EXT_meshopt_compression
  if (propertyDetails.enum && propertyDetails.enum.length > 0) {
    const firstEnumValue = propertyDetails.enum[0];

    return firstEnumValue !== undefined;
  }

  return false;
}

/**
 * If ... many conditions hold, ... then this will return the name
 * of the "initial" value of the given enum property.
 *
 * @param {Object} propertyDetails The property details
 * @param {Array} enums The enums
 * @returns The name of the default property
 */
function findNameOfInitial(propertyDetails, enums) {
  if (propertyDetails.default !== undefined) {
    for (let i = 0; i < enums.length; i++) {
      const element = enums[i];
      const enumValue = getEnumValue(element);
      if (enumValue === propertyDetails.default) {
        return createEnumIdentifier(element);
      }
    }
  }
  // No explicit default value was found. Return the first value
  for (let i = 0; i < enums.length; i++) {
    const element = enums[i];
    const enumValue = getEnumValue(element);
    if (enumValue !== undefined) {
      return createEnumIdentifier(element);
    }
  }
  return undefined;
}

function createEnumDefault(enumName, propertyDetails, enums) {
  return `${enumName}::${findNameOfInitial(propertyDetails, enums)}`;
}

function createEnum(enumDetails) {
  const enumValue = getEnumValue(enumDetails);
  if (enumValue === undefined) {
    return undefined;
  }

  if (enumDetails.type === "integer") {
    return `static constexpr int32_t ${createEnumIdentifier(
      enumDetails
    )} = ${enumValue}`;
  } else {
    return `inline static const std::string ${createEnumIdentifier(
      enumDetails
    )} = \"${enumValue}\"`;
  }
}

function createEnumIdentifier(enumDetails) {
  const enumValue = getEnumValue(enumDetails);
  if (enumValue === undefined) {
    return undefined;
  }

  if (enumDetails.type === "integer") {
    return makeIdentifier(
      makeNameIntoValidEnumIdentifier(enumDetails.description)
    );
  } else {
    return makeIdentifier(makeNameIntoValidEnumIdentifier(enumValue));
  }
}

function createEnumReaderType(parentName, enumName, propertyName, enums) {
  if (enums[0].type === "integer") {
    // No special reader needed for integer enums.
    return [];
  }
  if (findCommonEnumType(propertyName, enums) === "string") {
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

function createEnumReaderTypeImpl(parentName, enumName, propertyName, enums) {
  if (enums[0].type === "integer") {
    // No special reader needed for integer enums.
    return [];
  }
  if (findCommonEnumType(propertyName, enums) === "string") {
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
        enums
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

function makeNameIntoValidEnumIdentifier(name) {
  // May use this in the future to deconflict glTF enums from system header defines
  return name;
}

function createAnonymousPropertyTypeTitle(parentName, propertyName) {
  const propertyWithoutItems = toPascalCase(propertyName.replace(".items", ""));
  let result = parentName;
  if (!result.endsWith(propertyWithoutItems)) {
    result += " " + propertyWithoutItems;
  }
  result += " Value";

  return result;
}

module.exports = resolveProperty;
