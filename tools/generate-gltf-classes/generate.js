const path = require("path");
const fs = require("fs");
const lodash = require("lodash");
const resolveProperty = require("./resolveProperty");
const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");

function generate(schemaCache, nameMapping, outputDir, schema) {
  const name = getNameFromSchema(nameMapping, schema);

  console.log(`Generating ${name}`);
  // let baseSpecifier = "";
  // if (modelSchema.allOf && modelSchema.allOf.length > 0 && modelSchema.allOf[0].$ref) {
  //     switch (modelSchema.allOf[0].$ref) {
  //         case ""
  //     }
  // }

  const properties = Object.keys(schema.properties)
    .map((key) =>
      resolveProperty(
        schemaCache,
        nameMapping,
        key,
        schema.properties[key]
      )
    )
    .filter((property) => property !== undefined);

  const localTypes = lodash.uniq(lodash.flatten(properties.map(property => property.localTypes)));

  const headers = lodash.uniq(lodash.flatten(properties.map(property => property.headers)));
  headers.sort();

  const header = `
        #pragma once

        ${headers.map(header => `#include ${header}`).join("\n")}
        
        namespace CesiumGltf {
            struct ${name} {
                ${indent(localTypes.join("\n\n"), 16)}

                ${properties.map(property => `${property.type} ${property.name};`).join("\n\n                ")}
            };
        }
    `;

  const headerOutputDir = path.join(outputDir, "include", "CesiumGltf");
  fs.mkdirSync(headerOutputDir, { recursive: true });
  const headerOutputPath = path.join(headerOutputDir, name + ".h");
  fs.writeFileSync(headerOutputPath, unindent(header), "utf-8");

  return lodash.uniq(lodash.flatten(properties.map(property => property.schemas)));
}

function generateProperty(
  schemaCache,
  nameMapping,
  propertyName,
  propertyDetails
) {
  if (Object.keys(propertyDetails).length === 0) {
    return undefined;
  }

  const type = getPropertyType(schemaCache, nameMapping, propertyDetails);

  return type + " " + propertyName + ";";
}

function getPropertyType(schemaCache, nameMapping, propertyDetails) {
  if (propertyDetails.type == "array") {
    return `std::vector<${getPropertyType(
      schemaCache,
      nameMapping,
      propertyDetails.items
    )}>`;
  } else if (propertyDetails.type == "string") {
    return "std::string";
  } else if (propertyDetails.$ref) {
    const itemSchema = schemaCache.load(propertyDetails.$ref);
    if (itemSchema.title === "glTF Id") {
      return "int32_t";
    }
    return getNameFromSchema(nameMapping, itemSchema);
  } else if (propertyDetails.allOf && propertyDetails.allOf.length == 1) {
    return getPropertyType(schemaCache, nameMapping, propertyDetails.allOf[0]);
  }

  return "unknown";
}

module.exports = generate;
