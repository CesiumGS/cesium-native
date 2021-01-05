const yargs = require("yargs");
const path = require("path");
const SchemaCache = require("./SchemaCache");
const generate = require("./generate")

const argv = yargs.options({
  schema: {
    alias: "s",
    description: "The path to the glTF 2.0 JSONSchema files.",
    demandOption: true,
    type: "string"
  },
  output: {
    alias: "o",
    description: "The output directory for the generated files.",
    demandOption: true,
    type: "string"
  }
}).argv;

const nameMapping = {
  "glTF": "Model",
  "glTF Id": "ExtensibleObject",
  "glTFRootProperty": "NamedObject"
};

const schemaCache = new SchemaCache(argv.schema);
const modelSchema = schemaCache.load("glTF.schema.json");

const processed = {};

let schemas = [modelSchema];
while (schemas.length > 0) {
  const schema = schemas.pop();
  if (processed[schema.title]) {
    continue;
  }
  processed[schema.title] = true;
  schemas.push(...generate(schemaCache, nameMapping, argv.output, schema));
}
