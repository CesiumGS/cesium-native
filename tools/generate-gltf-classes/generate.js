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

  let base = "Extensible";
  if (schema.allOf && schema.allOf.length > 0 && schema.allOf[0].$ref && schema.allOf[0].$ref === "glTFChildOfRootProperty.schema.json") {
    base = "Named";
  }

  const properties = Object.keys(schema.properties)
    .map((key) =>
      resolveProperty(schemaCache, nameMapping, name, key, schema.properties[key])
    )
    .filter((property) => property !== undefined);

  const localTypes = lodash.uniq(
    lodash.flatten(properties.map((property) => property.localTypes))
  );

  const headers = lodash.uniq(
    [`"CesiumGltf/${base}Object.h"`, ...lodash.flatten(properties.map((property) => property.headers))]
  );
  headers.sort();

  const header = `
        #pragma once

        ${headers.map((header) => `#include ${header}`).join("\n")}
        
        namespace CesiumGltf {
            /**
             * @brief ${schema.description}
             */
            struct ${name} final : public ${base}Object {
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
    [`"${base}ObjectJsonHandler.h"`, ...lodash.flatten(properties.map((property) => property.readerHeaders))]
  );
  readerHeaders.sort();

  const readerLocalTypes = lodash.uniq(
    lodash.flatten(properties.map((property) => property.readerLocalTypes))
  );

  const readerHeader = `
        #pragma once

        ${readerHeaders.map((header) => `#include ${header}`).join("\n")}

        namespace CesiumGltf {
          struct ${name};

          class ${name}JsonHandler final : public ${base}ObjectJsonHandler {
          public:
            void reset(JsonHandler* pHandler, ${name}* pObject);
            virtual JsonHandler* Key(const char* str, size_t length, bool copy) override;

          private:
            ${indent(readerLocalTypes.join("\n\n"), 12)}

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

  const readerLocalTypesImpl = lodash.uniq(
    lodash.flatten(properties.map((property) => property.readerLocalTypesImpl))
  );

  const readerImpl = `
        #include "${name}JsonHandler.h"
        #include "CesiumGltf/${name}.h"
        #include <cassert>
        #include <string>

        using namespace CesiumGltf;

        void ${name}JsonHandler::reset(JsonHandler* pParent, ${name}* pObject) {
          ${base}ObjectJsonHandler::reset(pParent);
          this->_pObject = pObject;
        }

        JsonHandler* ${name}JsonHandler::Key(const char* str, size_t /*length*/, bool /*copy*/) {
          using namespace std::string_literals;

          assert(this->_pObject);

          ${indent(
            properties
              .map((property) => formatReaderPropertyImpl(property))
              .join("\n"),
            10
          )}

          return this->${base}ObjectKey(str, *this->_pObject);
        }

        ${indent(readerLocalTypesImpl.join("\n\n"), 8)}
  `;

  const readerSourceOutputPath = path.join(readerHeaderOutputDir, name + "JsonHandler.cpp");
  fs.writeFileSync(readerSourceOutputPath, unindent(readerImpl), "utf-8");

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

function formatReaderPropertyImpl(property) {
  return `if ("${property.name}"s == str) return property(this->_${property.name}, this->_pObject->${property.name});`;
}

module.exports = generate;
