const path = require("path");
const fs = require("fs");
const lodash = require("lodash");
const resolveProperty = require("./resolveProperty");
const getNameFromSchema = require("./getNameFromSchema");
const unindent = require("./unindent");
const indent = require("./indent");

function generate(options, schema) {
  const { schemaCache, nameMapping, outputDir, readerOutputDir } = options;

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
      resolveProperty(schemaCache, nameMapping, key, schema.properties[key])
    )
    .filter((property) => property !== undefined);

  const localTypes = lodash.uniq(
    lodash.flatten(properties.map((property) => property.localTypes))
  );

  const headers = lodash.uniq(
    lodash.flatten(properties.map((property) => property.headers))
  );
  headers.sort();

  const header = `
        #pragma once

        ${headers.map((header) => `#include ${header}`).join("\n")}
        
        namespace CesiumGltf {
            /**
             * @brief ${schema.description}
             */
            struct ${name} {
                ${indent(localTypes.join("\n\n"), 16)}

                ${indent(
                  properties
                    .map((property) => formatProperty(property))
                    .join("\n\n"),
                  16
                )}
            };
        }
    `;

  const headerOutputDir = path.join(outputDir, "include", "CesiumGltf");
  fs.mkdirSync(headerOutputDir, { recursive: true });
  const headerOutputPath = path.join(headerOutputDir, name + ".h");
  fs.writeFileSync(headerOutputPath, unindent(header), "utf-8");

  const readerHeaders = lodash.uniq(
    lodash.flatten(properties.map((property) => property.readerHeaders))
  );
  headers.sort();

  const readerHeader = `
        #pragma once

        ${readerHeaders.map((header) => `#include ${header}`).join("\n")}

        namespace CesiumGltf {
          class ${name}JsonHandler : public ObjectJsonHandler {
          public:
            void reset(JsonHandler* pHandler, ${name}* pObject);
            virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

          private:
            ${name}* _pObject;
            ${indent(
              properties
                .map((property) => formatReaderProperty(property))
                .join("\n"),
              12
            )}
          };
        }
  `;

  const readerHeaderOutputDir = path.join(readerOutputDir, "src");
  fs.mkdirSync(readerHeaderOutputDir, { recursive: true });
  const readerHeaderOutputPath = path.join(readerHeaderOutputDir, name + "JsonHandler.h");
  fs.writeFileSync(readerHeaderOutputPath, unindent(readerHeader), "utf-8");

  return lodash.uniq(
    lodash.flatten(properties.map((property) => property.schemas))
  );
}

function formatProperty(property) {
  let result = "";

  result += `/**\n * @brief ${property.briefDoc || property.name}\n`;
  if (property.fullDoc) {
    result += ` *\n * ${property.fullDoc.split("\n").join("\n * ")}\n`;
  }

  result += ` */\n`;

  result += `${property.type} ${property.name};`;

  return result;
}

function formatReaderProperty(property) {
  return `${property.readerType} _${property.name};`
}

module.exports = generate;
