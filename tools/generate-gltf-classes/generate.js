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

                ${indent(properties.map(property => formatProperty(property)).join("\n\n"), 16)}
            };
        }
    `;

  const headerOutputDir = path.join(outputDir, "include", "CesiumGltf");
  fs.mkdirSync(headerOutputDir, { recursive: true });
  const headerOutputPath = path.join(headerOutputDir, name + ".h");
  fs.writeFileSync(headerOutputPath, unindent(header), "utf-8");

  return lodash.uniq(lodash.flatten(properties.map(property => property.schemas)));
}

function formatProperty(property) {
  let result = "";

  result += `/**\n * @brief ${property.briefDoc || property.name}\n`;
  if (property.fullDoc) {
    result += ` *\n * ${property.fullDoc.split("\n").join("\n * ")}`;
  }

  result += ` */\n`;

  result += `${property.type} ${property.name};`;

  return result;
}

module.exports = generate;
