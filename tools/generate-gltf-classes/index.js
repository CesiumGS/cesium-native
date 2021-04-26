const yargs = require("yargs");
const path = require("path");
const fs = require("fs");
const SchemaCache = require("./SchemaCache");
const generate = require("./generate");

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
  },
  extensions: {
    alias: "e",
    description: "The extensions directory.",
    demandOption: true,
    type: "string"
  },
  config: {
    alias: "c",
    description: "The path to the configuration options controlling code generation, expressed in a JSON file.",
    demandOption: true,
    type: "string"
  }
}).argv;

const schemaCache = new SchemaCache(argv.schema, argv.extensions);
const modelSchema = schemaCache.load("glTF.schema.json");

const config = JSON.parse(fs.readFileSync(argv.config, "utf-8"));

const options = {
  schemaCache,
  outputDir: argv.output,
  readerOutputDir: argv.readerOutput,
  config: config,
  // key: Title of the element name that is extended (e.g. "Mesh Primitive")
  // value: Array of extension type names.
  extensions: {}
};

let schemas = [modelSchema];

for (const extension of config.extensions) {
  const extensionSchema = schemaCache.loadExtension(extension.schema);
  if (!extensionSchema) {
    console.warn(`Could not load schema ${extension.schema} for extension class ${extension.className}.`);
    continue;
  }

  if (!config.classes[extensionSchema.title]) {
    config.classes[extensionSchema.title] = {};
  }
  config.classes[extensionSchema.title].overrideName = extension.className;
  config.classes[extensionSchema.title].extensionName = extension.extensionName;

  schemas.push(...generate(options, extensionSchema));

  for (const objectToExtend of extension.attachTo) {
    const objectToExtendSchema = schemaCache.load(`${objectToExtend}.schema.json`);
    if (!objectToExtendSchema) {
      console.warn("Could not load schema for ${objectToExtend}.");
      continue;
    }

    if (!options.extensions[objectToExtend]) {
      options.extensions[objectToExtendSchema.title] = [];
    }

    options.extensions[objectToExtendSchema.title].push({
      name: extension.extensionName,
      className: extension.className
    });
  }
}

const processed = {};

while (schemas.length > 0) {
  const schema = schemas.pop();
  if (processed[schema.title]) {
    continue;
  }
  processed[schema.title] = true;
  schemas.push(...generate(options, schema));
}
