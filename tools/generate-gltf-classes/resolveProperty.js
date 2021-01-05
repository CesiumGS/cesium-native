const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");

function resolveProperty(schemaCache, nameMapping, propertyName, propertyDetails) {
    if (Object.keys(propertyDetails).length === 0) {
        // Ignore totally empty properties.
        return undefined;
    }

    const result = {
        headers: [],
        type: "",
        name: propertyName,
        handlerType: "",
        schemas: [],
        localTypes: []
    };

    if (propertyDetails.type == "array") {
        result.headers.push("<vector>");
        
        const itemProperty = resolveProperty(schemaCache, nameMapping, propertyName + ".items", propertyDetails.items);
        if (!itemProperty) {
            return undefined;
        }

        result.headers.push(...itemProperty.headers);
        result.schemas.push(...itemProperty.schemas);
        result.localTypes.push(...itemProperty.localTypes);
        result.type = `std::vector<${itemProperty.type}>`;
    } else if (propertyDetails.type == "integer") {
        result.headers.push("<cstdint>");
        result.type = "int64_t";
    } else if (propertyDetails.type == "number") {
        result.type = "double";
    } else if (propertyDetails.type == "boolean") {
        result.type = "bool";
    } else if (propertyDetails.type == "string") {
        result.headers.push("<string>");
        result.type = "std::string";
    } else if  (propertyDetails.type == "object" && propertyDetails.additionalProperties) {
        const additionalPropertiesProperty = resolveProperty(schemaCache, nameMapping, propertyName + ".additionalProperties", propertyDetails.additionalProperties);
        if (!additionalPropertiesProperty) {
            return undefined;
        }

        result.headers.push(...additionalPropertiesProperty.headers);
        result.schemas.push(...additionalPropertiesProperty.schemas);
        result.localTypes.push(...additionalPropertiesProperty.localTypes);

        result.headers.push("<unordered_map>");
        result.type = `std::unordered_map<std::string, ${additionalPropertiesProperty.type}>`;
    } else if (propertyDetails.anyOf && propertyDetails.anyOf.length > 0 && propertyDetails.anyOf[0].enum) {
        const enumName = toPascalCase(propertyName);
        result.localTypes.push(unindent(`
            enum class ${toPascalCase(propertyName)} {
                ${indent(propertyDetails.anyOf.map(e => createEnum(e)).filter(e => e !== undefined).join(",\n\n"), 16)}
            };`
        ));
        result.type = enumName;
    } else if (propertyDetails.$ref) {
        const itemSchema = schemaCache.load(propertyDetails.$ref);
        if (itemSchema.title === "glTF Id") {
            result.headers.push("<cstdint>");
            result.type = "int32_t";
        } else {
            result.type = getNameFromSchema(nameMapping, itemSchema);
            result.headers.push(`"CesiumGltf/${result.type}.h"`);
            result.schemas.push(itemSchema);
        }
    } else if (propertyDetails.allOf && propertyDetails.allOf.length == 1) {
        return resolveProperty(schemaCache, nameMapping, propertyName, propertyDetails.allOf[0]);
    } else {
        console.warn(`Skipping unhandled property ${propertyName}.`);
        return undefined;    
    }

    return result;
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

    if (typeof enumDetails.enum[0] === 'string') {
        return enumDetails.enum[0];
    } else {
        return `${enumDetails.description} = ${enumDetails.enum[0]}`;
    }
}

module.exports = resolveProperty;