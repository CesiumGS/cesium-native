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
    description: "The output directory for the generated glTF class files.",
    demandOption: true,
    type: "string"
  },
  readerOutput: {
    alias: "r",
    description: "The output directory for the generated reader files.",
    demandOption: true,
    type: "string"
  }
}).argv;

const nameMapping = {
  "glTF": "Model",
  "glTF Id": "ExtensibleObject",
  "glTFRootProperty": "NamedObject"
};

// Custom `cesium` properties will be added to these objects.
// e.g. Buffer will gain `BufferCesium cesium;`
const cesiumProperties = {
  "Buffer": true,
  "Image": true
};

const schemaCache = new SchemaCache(argv.schema);
const modelSchema = schemaCache.load("glTF.schema.json");

const options = {
  schemaCache,
  nameMapping,
  cesiumProperties,
  outputDir: argv.output,
  readerOutputDir: argv.readerOutput
};

const processed = {};

let schemas = [modelSchema];
while (schemas.length > 0) {
  const schema = schemas.pop();
  if (processed[schema.title]) {
    continue;
  }
  processed[schema.title] = true;
  schemas.push(...generate(options, schema));
}
