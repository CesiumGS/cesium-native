const yargs = require("yargs");
const path = require("path");
const fs = require("fs");
const SchemaCache = require("./SchemaCache");
const generate = require("./generate");

const argv = yargs.options({
  schema: {
    alias: "s",
    description: "The path to the root JSONSchema.",
    demandOption: true,
    type: "string"
  },
  output: {
    alias: "o",
    description: "The output directory for the generated glTF class files.",
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
  },
  namespace: {
    alias: "n",
    description: "Namespace to put the generated classes/methods in.",
    demandOption: true,
    type: "string"
  }
}).argv;

const schemaPath = path.dirname(argv.schema) + path.sep;
const rootSchema = path.basename(argv.schema);

const config = JSON.parse(fs.readFileSync(argv.config, "utf-8"));

const options = {
  outputDir: argv.output,
  namespace: argv.namespace,
};

for (const extension of config.extensions) {
  const extPath = path.dirname(extension.schema) + path.sep;
  const extRoot = path.basename(extension.schema);
  const overrides = {};
  overrides[extRoot] = {
    name: extension.name
  };
  const extOptions = {
    ...options,
    extension: extension.name,
    overrides: overrides
  };
  generate(extOptions, extRoot, path.join(argv.extensions, extPath), schemaPath);
}

generate({...options, overrides: config.overrides}, rootSchema, schemaPath);

